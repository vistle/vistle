#include "sensei.h"
#include "exeption.h"
#include "intOption.h"

#include <vistle/insitu/core/prepareShmName.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/addObjectMsq.h>
#include <vistle/util/hostname.h>

#include <cassert>
#include <fstream>
#include <iostream>

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
struct Internals {
    message::ModuleInfo moduleInfo;
    std::unique_ptr<vistle::insitu::message::AddObjectMsq>
    sendMessageQueue; // Queue to send addObject messages to module

    insitu::message::SyncShmIDs shmIDs;
    message::InSituShmMessage messageHandler;
    std::set<IntOption> intOptions; // options that can be set in the module
};
} // namespace detail
} // namespace sensei
} // namespace insitu
} // namespace vistle

SenseiAdapter::SenseiAdapter(bool paused, MPI_Comm Comm, MetaData &&meta, ObjectRetriever cbs)
: m_callbacks(cbs), m_metaData(std::move(meta)), m_internals(new detail::Internals{})
{
    MPI_Comm_rank(Comm, &m_rank);
    MPI_Comm_size(Comm, &m_mpiSize);
    m_commands["run/paused"] = !paused; // if true run else wait
    m_commands["exit"] = false; // let the simulation know that vistle wants to
    // exit by returning false from execute
    try {
        m_internals->messageHandler.initialize(m_rank);
        dumpConnectionFile(comm);
    } catch (...) {
        throw Exeption() << "failed to create connection facilities for Vistle";
    }
    m_internals->intOptions.insert(IntOption{IntOptions::KeepTimesteps, true});
    m_internals->intOptions.insert(IntOption{IntOptions::NthTimestep, 1});
#ifdef MODULE_THREAD
    startVistle(comm);
#endif
}

SenseiAdapter::~SenseiAdapter()
{
    delete m_internals;
    m_internals->messageHandler.send(message::ConnectionClosed{true});
#ifdef MODULE_THREAD
    CERR << "Quit requested, waiting for vistle manager to finish" << endl;
    m_managerThread.join();
#endif
}

bool SenseiAdapter::Execute(size_t timestep)
{
    if (stillConnected() && !quitRequested() && WaitedForModuleCommands()) {
        if (m_internals->moduleInfo.isReady() && haveToProcessTimestep(timestep)) {
            processData();
        }
        return true;
    }
    return false;
}

#ifdef MODULE_THREAD
bool SenseiAdapter::startVistle(const MPI_Comm &comm)
{
    m_managerThread = std::thread([this]() {
        const char *VISTLE_ROOT = getenv("VISTLE_ROOT");
        if (!VISTLE_ROOT) {
            CERR << "VISTLE_ROOT not set to the path of the Vistle build directory." << endl;
            return false;
        }
        std::string cmd{VISTLE_ROOT};
        cmd += "/bin/vistle_manager";
        std::vector<char *> args;
        args.push_back(const_cast<char *>(cmd.c_str()));
        vistle::VistleManager manager;
        manager.run(1, args.data());
        return true;
    });
    return true;
}

#endif

bool SenseiAdapter::stillConnected()
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

bool SenseiAdapter::quitRequested()
{
    if (m_commands["exit"]) {
        m_internals->messageHandler.send(ConnectionClosed{true});
#ifdef MODULE_THREAD
        m_managerThread.join();
#endif
        return true;
    }
    return false;
}

bool SenseiAdapter::WaitedForModuleCommands()
{
    auto it = m_commands.find("run/paused");
    while (!it->second) // also let the simulation wait for the module if
    // initialized with paused
    {
        recvAndHandeMessage(true);
        if (quitRequested() || !m_connected) {
            CERR << "sensei controller disconnected" << endl;
            return false;
        }
    }
    return true;
}

bool SenseiAdapter::haveToProcessTimestep(size_t timestep)
{
    return timestep % m_internals->intOptions.find(IntOptions::NthTimestep)->value() == 0;
}

void SenseiAdapter::processData()
{
    auto dataObjects = m_callbacks.getData(m_usedData);
    for (const auto dataObject: dataObjects) {
        bool keepTimesteps = m_internals->intOptions.find(IntOptions::KeepTimesteps)->value();
        keepTimesteps ? dataObject.object()->setTimestep(m_processedTimesteps)
                      : dataObject.object()->setIteration(m_processedTimesteps);
        addObject(dataObject.portName(), dataObject.object());
    }
    ++m_processedTimesteps;
}

bool SenseiAdapter::Finalize()
{
    CERR << "Finalizing" << endl;
    if (m_internals->moduleInfo.isInitialized()) {
        m_internals->messageHandler.send(ConnectionClosed{true});
    }
    return true;
}

void SenseiAdapter::calculateUsedData()
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

bool SenseiAdapter::objectRequested(const std::string &name, const std::string &meshName)
{
    return m_internals->moduleInfo.isPortConnected(name) ||
           m_internals->moduleInfo.isPortConnected(portName(meshName, name));
}

void SenseiAdapter::dumpConnectionFile(MPI_Comm Comm)
{
    std::vector<std::string> names;
    boost::mpi::gather(boost::mpi::communicator(comm, boost::mpi::comm_attach),
                       m_internals->messageHandler.name(), names, 0);
    if (m_rank == 0) {
        std::ofstream outfile("sensei.vistle");
        for (int i = 0; i < m_mpiSize; i++) {
            outfile << std::to_string(i) << " " << names[i] << endl;
        }
        outfile.close();
    }
}

bool SenseiAdapter::recvAndHandeMessage(bool blocking)
{
    message::Message msg =
    blocking ? m_internals->messageHandler.recv() : m_internals->messageHandler.tryRecv();
    m_internals->moduleInfo.update(msg);
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        return false;
    case InSituMessageType::ShmInfo: {
        return initModule(msg);
    } break;
    case InSituMessageType::SetPorts: {
        calculateUsedData();
    } break;
    case InSituMessageType::Ready: {
        m_processedTimesteps = 0;
        if (m_internals->moduleInfo.isReady()) {
            vistle::Shm::the().setObjectID(m_internals->shmIDs.objectID());
            vistle::Shm::the().setArrayID(m_internals->shmIDs.arrayID());
        } else {
            m_internals->shmIDs.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID());
            m_internals->messageHandler.send(
            Ready{false}); // confirm that we are done creating vistle objects
        }
    } break;
    case InSituMessageType::ExecuteCommand: {
        ExecuteCommand exe = msg.unpackOrCast<ExecuteCommand>();
        auto it = m_commands.find(exe.value);
        if (it != m_commands.end()) {
            it->second = !it->second;
        } else {
            CERR << "receive unknow command: " << exe.value << endl;
        }
    } break;
    case InSituMessageType::ConnectionClosed: {
        m_internals->sendMessageQueue.reset(nullptr);
        m_connected = false;
        m_internals->shmIDs.close();
        CERR << "connection closed" << endl;
    } break;
    case InSituMessageType::SenseiIntOption: {
        auto option = msg.unpackOrCast<SenseiIntOption>().value;
        update(m_internals->intOptions, option);
    } break;
    default:
        break;
    }
    return true;
}

bool SenseiAdapter::initModule(const Message &msg)
{
    if (m_connected) {
        CERR << "warning: received connection attempt but we are already "
                "connectted with module "
             << m_internals->moduleInfo.name() << m_internals->moduleInfo.id() << endl;
        return false;
    }
    if (!checkHostName() || !checkMpiSize() || !initializeVistleEnv()) {
        m_internals->messageHandler.send(ConnectionClosed{true});
        return false;
    }
    addCommands();
    addPorts();
    m_internals->moduleInfo.setInitState(true);
    return true;
}

bool SenseiAdapter::checkHostName() const
{
    if (m_rank == 0 && m_internals->moduleInfo.hostname() != vistle::hostname()) {
        CERR << "this " << vistle::hostname() << "trying to connect to "
             << m_internals->moduleInfo.hostname() << endl;
        CERR << "Wrong host: must connect to Vistle on the same machine!" << endl;
        return false;
    }
    return true;
}

bool SenseiAdapter::checkMpiSize() const
{
    if (static_cast<size_t>(m_mpiSize) != m_internals->moduleInfo.mpiSize()) {
        CERR << "Vistle's mpi = " << m_internals->moduleInfo.mpiSize()
             << " and this mpi size = " << m_mpiSize << " do not match" << endl;
        return false;
    }
    return true;
}

bool SenseiAdapter::initializeVistleEnv()
{
    try {
#ifndef MODULE_THREAD
        vistle::registerTypes();
#endif
        initializeMessageQueues();
    } catch (const vistle::exception &ex) {
        CERR << ex.what() << endl;
        return false;
    }

    m_connected = true;
    CERR << "connection to module " << m_internals->moduleInfo.name()
         << m_internals->moduleInfo.id() << " established!" << endl;
    return true;
}

void SenseiAdapter::initializeMessageQueues() throw()
{
    auto shmName = cutRankSuffix(m_internals->moduleInfo.shmName());
    CERR << "attaching to shm: name = " << shmName << " id = " << m_internals->moduleInfo.id()
         << " rank = " << m_rank << endl;
    vistle::Shm::attach(shmName, m_internals->moduleInfo.id(), m_rank);

    m_internals->sendMessageQueue.reset(new AddObjectMsq(m_internals->moduleInfo, m_rank));

    m_internals->shmIDs.initialize(m_internals->moduleInfo.id(), m_rank,
                                   m_internals->moduleInfo.uniqueSuffix(),
                                   SyncShmIDs::Mode::Attach);
}

std::string SenseiAdapter::portName(const std::string &meshName, const std::string &varName)
{
    if (varName.empty()) {
        return meshName;
    }
    return meshName + "_" + varName;
}

void SenseiAdapter::addCommands()
{
    std::vector<std::string> commands;
    for (const auto &command: m_commands) {
        commands.push_back(command.first);
    }
    m_internals->messageHandler.send(SetCommands{commands});
}

void SenseiAdapter::addPorts()
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
    m_internals->messageHandler.send(SetPorts{ports});
}

void SenseiAdapter::addObject(const std::string &port, vistle::Object::const_ptr obj)
{
    if (m_internals->sendMessageQueue) {
        m_internals->sendMessageQueue->addObject(port, obj);
    } else {
        CERR << "VistleSenseiAdapter can not add vistle object: sendMessageQueue = "
                "null"
             << endl;
    }
}

bool SenseiAdapter::isMOduleReady()
{
    return m_internals->moduleInfo.isReady();
}
void SenseiAdapter::updateShmIDs()
{
    m_internals->shmIDs.set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID());
}