#include "sensei.h"
#include "exception.h"

#include <vistle/insitu/core/attachVistleShm.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/addObjectMsq.h>
#include <vistle/insitu/message/sharedParam.h>

#include <vistle/util/directory.h>
#include <vistle/util/enumarray.h>
#include <vistle/util/filesystem.h>
#include <vistle/util/hostname.h>
#include <vistle/util/shmconfig.h>

#include <cassert>
#include <fstream>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/mpi/collectives.hpp>

#ifdef MODULE_THREAD
#include <vistle/manager/manager.h>
#endif // MODULE_THREAD

#define CERR std::cerr << "[" << m_rank << "/" << m_mpiSize << " ] SenseiAdapter: "
using std::endl;
using namespace vistle::insitu;
using namespace vistle::insitu::sensei;
using namespace vistle::insitu::message;

namespace vistle {
namespace insitu {
namespace sensei {
namespace detail {

int getRank(MPI_Comm Comm)
{
    int rank = 0;
    MPI_Comm_rank(Comm, &rank);
    return rank;
}

struct Internals {
    Internals(MPI_Comm comm): messageHandler(std::make_unique<message::InSituShmMessage>(comm)) {}
    message::ModuleInfo moduleInfo;
    std::unique_ptr<message::AddObjectMsq> sendMessageQueue; // Queue to send addObject messages to module

    std::unique_ptr<message::InSituShmMessage> messageHandler;
    // options that can be set in the module
    std::vector<message::IntParam> moduleParams{{"frequency", 1}, {"keep_timesteps", false}};
};
} // namespace detail
} // namespace sensei
} // namespace insitu
} // namespace vistle

Adapter::Adapter(bool paused, MPI_Comm Comm, MetaData &&meta, ObjectRetriever cbs, const std::string &vistleRoot,
                 const std::string &vistleBuildType, const std::string &options)
: m_callbacks(std::move(cbs)), m_metaData(std::move(meta)), m_internals(new detail::Internals{Comm}), m_comm(Comm)
{
    MPI_Comm_rank(Comm, &m_rank);
    MPI_Comm_size(Comm, &m_mpiSize);
    m_commands["run_simulation"] = !paused; // if true run else wait
    // exit by returning false from execute
    dumpConnectionFile(comm);

#ifdef MODULE_THREAD
    vistle::directory::setVistleRoot(vistleRoot, vistleBuildType);
    startVistle(comm, options);
#else
    (void)vistleRoot;
    (void)vistleBuildType;
#endif
}

bool Adapter::execute(size_t timestep)
{
    auto tStart = vistle::Clock::time();
    if (stillConnected() && waitedForModuleCommands()) {
        if (haveToProcessTimestep(timestep)) {
            static bool first = true;
            if (first) {
                first = false;
                m_timeSpendInExecute = 0;
                m_startTime = vistle::Clock::time();
                tStart = m_startTime;
            }
            processData();
        }
    }
    m_timeSpendInExecute += vistle::Clock::time() - tStart;
    return true;
}

#ifdef MODULE_THREAD
bool Adapter::startVistle(const MPI_Comm &comm, const std::string &options)
{
    int prov = MPI_THREAD_SINGLE;
    MPI_Query_thread(&prov);
    if (prov != MPI_THREAD_MULTIPLE) {
        CERR << "startVistle: MPI_THREAD_MULTIPLE not provided" << std::endl;
        return false;
    }
    const char *VISTLE_ROOT = getenv("VISTLE_ROOT");
    if (!VISTLE_ROOT) {
        CERR << "VISTLE_ROOT not set to the path of the Vistle build directory." << endl;
        return false;
    }
    m_managerThread = std::thread([VISTLE_ROOT, options]() {
        std::string cmd{VISTLE_ROOT};
        cmd += "/bin/vistle_manager";
        std::vector<const char *> args;
        std::vector<std::string> optionsVec;
        if (!options.empty())
            boost::split(optionsVec, options, boost::is_any_of(" "));
        std::cerr << "options are: " << options << "!" << std::endl;
        args.push_back(cmd.c_str());
        for (auto &opt: optionsVec) {
            args.push_back(opt.c_str());
        }
        args.push_back("--root");
        args.push_back(VISTLE_ROOT);
        vistle::VistleManager manager;
        manager.run(static_cast<int>(args.size()), const_cast<char **>(args.data()));
    });
    return true;
}

#endif

bool Adapter::stillConnected()
{
    bool wasConnected = m_connected;
    while (recvAndHandeMessage()) {
    } // catch newest state
    if (wasConnected && !m_connected) {
        CERR << "sensei controller disconnected" << endl;
        return false;
    }
    return true;
}

void Adapter::restart()
{
    CERR << "connection closed" << endl;
    insitu::detachShm();
    delete m_internals;
    m_internals = new detail::Internals{m_comm};
    m_connected = false;
}

bool Adapter::waitedForModuleCommands()
{
    auto it = m_commands.find("run_simulation");
    while (!it->second) // also let the simulation wait for the module if
    // initialized with paused
    {
        recvAndHandeMessage(true);
        if (!m_connected) {
            CERR << "sensei controller disconnected" << endl;
            return false;
        }
    }
    return true;
}

bool Adapter::haveToProcessTimestep(size_t timestep)
{
    return (timestep % message::getIntParamValue(m_internals->moduleParams, "frequency")) == 0;
}

void Adapter::processData()
{
    if (!m_internals->sendMessageQueue) {
        CERR << "VistleSenseiAdapter can not add vistle object: sendMessageQueue = "
                "null"
             << endl;
        return;
    }

    auto dataObjects = m_callbacks.getData(m_usedData);
    for (const auto &dataObject: dataObjects) {
        m_internals->sendMessageQueue->addObject(dataObject.portName(), dataObject.object());
    }
    m_internals->sendMessageQueue->sendObjects();
    if (message::getIntParamValue(m_internals->moduleParams, "keep_timesteps"))
        ++m_processedTimesteps;
    else
        ++m_iteration;
}

Adapter::~Adapter()
{
    if (m_internals)
        finalize();
}

bool Adapter::finalize()
{
    CERR << "Finalizing" << endl;
    double averageTimeSpendInExecute = 0;
    MPI_Reduce(&m_timeSpendInExecute, &averageTimeSpendInExecute, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    auto simulationTime = Clock::time() - m_startTime;
    double averageSimTime = 0;
    MPI_Reduce(&simulationTime, &averageSimTime, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    if (m_rank == 0) {
        std::cerr << "simulation took " << averageSimTime / m_mpiSize << "s" << std::endl;
        std::cerr << "avarage time spend in execute: " << averageTimeSpendInExecute / m_mpiSize << "s" << std::endl;
    }
    if (m_internals->moduleInfo.isInitialized()) {
        m_internals->messageHandler->send(ConnectionClosed{true});
    }
#ifdef MODULE_THREAD
    if (m_managerThread.joinable()) {
        CERR << "Quit requested, waiting for vistle manager to finish" << endl;
        m_managerThread.join();
    }
#endif

    delete m_internals;
    m_internals = nullptr;
    return true;
}

void Adapter::calculateUsedData()
{
    m_usedData = MetaData{};

    for (const auto &simMesh: m_metaData) {
        MetaMesh mm(simMesh.name());
        for (const auto &simVariable: simMesh) {
            if (objectRequested(simVariable, simMesh.name())) {
                mm.addVar(simVariable);
            }
        }
        if (!mm.empty() || objectRequested(simMesh.name())) {
            m_usedData.addMesh(mm);
        }
    }
}

bool Adapter::objectRequested(const std::string &name, const std::string &meshName)
{
    return m_internals->moduleInfo.isPortConnected(name) ||
           m_internals->moduleInfo.isPortConnected(portName(meshName, name));
}

void Adapter::dumpConnectionFile(MPI_Comm Comm)
{
    std::vector<std::string> names;
    boost::mpi::gather(boost::mpi::communicator(comm, boost::mpi::comm_attach), m_internals->messageHandler->name(),
                       names, 0);
    if (m_rank == 0) {
        if (!vistle::filesystem::exists(vistle::directory::configHome())) {
            vistle::filesystem::create_directory(vistle::directory::configHome());
        }
        std::ofstream outfile(directory::configHome() + "/sensei.vistle");
        for (int i = 0; i < m_mpiSize; i++) {
            outfile << std::to_string(i) << " " << names[i] << endl;
        }
        outfile.close();
    }
}

bool Adapter::recvAndHandeMessage(bool blocking)
{
    message::Message msg = blocking ? m_internals->messageHandler->recv() : m_internals->messageHandler->tryRecv();
    m_internals->moduleInfo.update(msg);
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        return false;
    case InSituMessageType::ShmInfo: {
        return initModule(msg);
    } break;
    case InSituMessageType::SetPorts:
    case InSituMessageType::ConnectPort:
    case InSituMessageType::DisconnectPort: {
        calculateUsedData();
    } break;
    case InSituMessageType::ExecuteCommand: {
        ExecuteCommand exe = msg.unpackOrCast<ExecuteCommand>();
        auto it = m_commands.find(exe.value.first);
        if (it != m_commands.end()) {
            it->second = !it->second;
        } else {
            CERR << "received unknown command: " << exe.value.first << endl;
        }
    } break;
    case InSituMessageType::ConnectionClosed: {
        auto state = msg.unpackOrCast<ConnectionClosed>();
        if (state.value == message::DisconnectState::ShutdownNoRestart)
            finalize();
        else
            restart();
    } break;
    case InSituMessageType::IntOption: {
        auto option = msg.unpackOrCast<IntOption>().value;
        updateIntParam(m_internals->moduleParams, option);
        if (option.name == "keep_timesteps") {
            m_processedTimesteps = -1;
            m_iteration = -1;
            if (option.value == 0) {
                m_iteration = 0;
            } else {
                m_processedTimesteps = 0;
            }
        }
        ++m_generation;

    } break;
    default:
        break;
    }
    return true;
}

bool Adapter::initModule(const Message &msg)
{
    if (m_connected) {
        CERR << "warning: received connection attempt, but we are already "
                "connected with module "
             << m_internals->moduleInfo.name() << m_internals->moduleInfo.id() << endl;
        return false;
    }
    //m_internals->moduleInfo() is already updated with the new shm info
    if (!checkHostName() || !checkMpiSize() || !initializeVistleEnv()) {
        m_internals->messageHandler->send(ConnectionClosed{true});
        return false;
    }
    addCommands();
    addPorts();
    m_internals->moduleInfo.setInitState(true);
    return true;
}

bool Adapter::checkHostName() const
{
    if (m_rank == 0 && m_internals->moduleInfo.hostname() != vistle::hostname()) {
        CERR << "this " << vistle::hostname() << "trying to connect to " << m_internals->moduleInfo.hostname() << endl;
        CERR << "Wrong host: must connect to Vistle on the same machine!" << endl;
        return false;
    }
    return true;
}

bool Adapter::checkMpiSize() const
{
    if (static_cast<size_t>(m_mpiSize) != m_internals->moduleInfo.mpiSize()) {
        CERR << "Vistle's mpi = " << m_internals->moduleInfo.mpiSize() << " and this mpi size = " << m_mpiSize
             << " do not match" << endl;
        return false;
    }
    return true;
}

bool Adapter::initializeVistleEnv()
{
    try {
#ifndef MODULE_THREAD
        static bool typesRegistered = false;
        if (!typesRegistered) {
            vistle::registerTypes(); //must not be called more than once per process
            typesRegistered = true;
        }
#endif
        initializeMessageQueues();
    } catch (const vistle::exception &ex) {
        CERR << ex.what() << endl;
        return false;
    }

    m_connected = true;
    CERR << "connection to module " << m_internals->moduleInfo.name() << m_internals->moduleInfo.id() << " established!"
         << endl;
    return true;
}

void Adapter::initializeMessageQueues() throw()
{
    insitu::attachShm(m_internals->moduleInfo.shmName(), m_internals->moduleInfo.id(), m_rank);
    m_internals->sendMessageQueue.reset(new AddObjectMsq(m_internals->moduleInfo, m_rank));
}

std::string Adapter::portName(const std::string &meshName, const std::string &varName)
{
    if (varName.empty()) {
        return meshName;
    }
    return meshName + "_" + varName;
}

void Adapter::addCommands()
{
    std::vector<std::string> commands;
    for (const auto &command: m_commands) {
        commands.push_back(command.first);
    }
    m_internals->messageHandler->send(SetCommands{commands});
}

void Adapter::addPorts()
{
    std::vector<std::vector<std::string>> ports{2};
    for (const auto &mesh: m_metaData) {
        ports[0].push_back(mesh.name());
        for (const auto &var: mesh) {
            ports[1].push_back(portName(mesh.name(), var));
        }
    }
    ports[0].push_back("mesh");
    ports[1].push_back("variable");
    m_internals->messageHandler->send(SetPorts{ports});
}

void Adapter::updateMeta(vistle::Object::ptr obj) const
{
    if (obj) {
        obj->setCreator(m_internals->moduleInfo.id());
        obj->setGeneration(m_generation);
        obj->setTimestep(m_processedTimesteps);
        obj->setIteration(m_iteration);
        obj->updateInternals();
    }
}

bool Adapter::paused() const
{
    return !m_commands.at("run_simulation");
}
