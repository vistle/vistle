#include "Engine.h"

#include "Exception.h"
#include "VertexTypesToVistle.h"
#include "VisitDataTypesToVistle.h"

#include <vistle/insitu/libsim/libsimInterface/SimulationMetaData.h>
#include <vistle/module/module.h>

#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/insitu/core/attachVistleShm.h>
#include <vistle/util/directory.h>
#include <vistle/util/hostname.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/shmconfig.h>
#include <vistle/util/sleep.h>
#include <vistle/util/stopwatch.h>

#include <boost/asio/connect.hpp>
#include <boost/mpi.hpp>

#include <ostream>
#include <chrono>

#ifdef MODULE_THREAD
#include "engineInterface/EngineInterface.cpp"
#include <vistle/manager/manager.h>
#endif // MODULE_THREAD

#ifdef LIBSIM_DEBUG
#define DEBUG_CERR \
    std::cerr << "Engine: " \
              << " [" << m_rank << "/" << m_mpiSize << "] "
#else
#include <vistle/insitu/util/print.h>
#define DEBUG_CERR vistle::DoNotPrintInstance
#endif
#define CERR \
    std::cerr << "Engine: " \
              << " [" << m_rank << "/" << m_mpiSize << "] "

using std::endl;
using std::string;
using std::vector;
using namespace vistle::insitu;
using namespace vistle::insitu::message;
using namespace vistle::insitu::libsim;
namespace asio = boost::asio;
Engine *Engine::instance = nullptr;

Engine *Engine::EngineInstance()
{
    if (!instance) {
        instance = new Engine{};
    }
    return instance;
}

void Engine::DisconnectSimulation()
{
    std::cerr << "Enigne [" << instance->m_rank << "/" << instance->m_mpiSize << "] disconnecting..." << endl;
    auto msg = message::ConnectionClosed{true};
    if (instance->m_messageHandler)
        instance->m_messageHandler->send(msg);
    delete instance;
    instance = nullptr;
    insitu::detachShm();
}

bool Engine::initialize(int argC, char **argV)
{
    if (m_initialized) {
        CERR << "initialize called, but engine is already initialized" << endl;
        return false;
    }


#ifdef MODULE_THREAD
    return startVistle(argC, argV);
#else
    if (checkInitArgs(argC, argV)) {
        m_modulePort = atoi(argV[0]);
        m_initialized = connectToVistleModule();
        return m_initialized;
    }
    return true;
#endif
}

bool Engine::isInitialized() const noexcept
{
    return m_initialized;
}

bool Engine::setMpiComm(void *newconn)
{
    comm = *static_cast<MPI_Comm *>(newconn);
    MPI_Comm_rank(comm, &m_rank);
    MPI_Comm_size(comm, &m_mpiSize);
    return true;
}

bool Engine::sendData()
{
    CERR << "sendData was called" << endl;
    return true;
}

void Engine::SimulationTimeStepChanged()
{
    if (!m_initialized || !m_moduleInfo.isInitialized()) {
        CERR << "not connected with Vistle. \nStart the ConnectLibSIm module to use simulation data in Vistle!" << endl;
        return;
    }

    m_metaData.fetchFromSim();
    static size_t processedCycles = 0;
    if (processedCycles > 0 && processedCycles >= m_metaData.currentCycle()) {
        DEBUG_CERR << "There is no new timestep but SimulationTimeStepChanged was called" << endl;
        return;
    }
    int numMeshes, numVars;
    v2check(simv2_SimulationMetaData_getNumMeshes, m_metaData.handle(), numMeshes);
    v2check(simv2_SimulationMetaData_getNumVariables, m_metaData.handle(), numVars);
    if (m_metaData.currentCycle() % m_rules.frequency) {
        return;
    }
    static std::unique_ptr<StopWatch> watch;
    watch = std::make_unique<StopWatch>(("Engine: [" + std::to_string(m_rank) + "/" + std::to_string(m_mpiSize) +
                                         "]: time since last processed timestep")
                                            .c_str());
    DEBUG_CERR << "Timestep " << m_metaData.currentCycle() << " has " << numMeshes << " meshes and " << numVars
               << "variables" << endl;
    sendObjectsToModule();

    processedCycles = m_metaData.currentCycle();
    if (m_rules.keepTimesteps)
        ++m_processedTimesteps;
    else
        ++m_iterations;
}

void Engine::SimulationInitiateCommand(const string &command)
{
    static int counter = 0;
    DEBUG_CERR << "SimulationInitiateCommand " << command << " counter = " << counter << endl;
    ++counter;
    if (command.substr(0, 12) ==
        "INTERNALSYNC") { // need to respond or LibSim gets stuck. see: SimEngine::SimulationInitiateCommand
        // Send the command back to the engine so it knows we're done syncing.
        string cmd("INTERNALSYNC");
        string args(command.substr(13, command.size() - 1));
        simulationCommandCallback(cmd.c_str(), args.c_str(), simulationCommandCallbackData);
        m_messageHandler->send(GoOn{}); // request tcp message from controller
    }
}

void Engine::DeleteData()
{
    DEBUG_CERR << "DeleteData called" << endl;
    resetDataTransmitter();
    sendData();
}

bool Engine::fetchNewModuleState()
{
    if (m_rank == 0) {
        if (!slaveCommandCallback) {
            CERR << "passCommandToSim failed : slaveCommandCallback not set" << endl;
            return true;
            ;
        }
        slaveCommandCallback(); // let the slaves call visitProcessEngineCommand() -> simv2_process_input() ->
            // Engine::passCommandToSim() and therefore finalizeInit() if not already done
    }
    m_metaData.fetchFromSim();
    initializeSim();
    auto msg = m_messageHandler->recv();
    DEBUG_CERR << "received message of type " << static_cast<int>(msg.type()) << endl;
    m_moduleInfo.update(msg);
    switch (msg.type()) {
    case InSituMessageType::ShmInfo:

        if (m_moduleInfo.mpiSize() != static_cast<size_t>(m_mpiSize)) {
            CERR << "mpi size of simulation must match vistle's mpi size" << endl;
            return false;
        }
        if (m_moduleInfo.hostname() != vistle::hostname()) {
            CERR << "Simulation on " << vistle::hostname() << " is trying to connect to Vistle on "
                 << m_moduleInfo.hostname() << "." << endl;
            CERR << "Connection to non local host is not supported!" << endl;
            return false;
        }
        insitu::attachShm(m_moduleInfo.shmName(), m_moduleInfo.id(), m_rank);
        resetDataTransmitter();
        break;
    case InSituMessageType::ExecuteCommand: {
        ExecuteCommand exe = msg.unpackOrCast<ExecuteCommand>();
        if (simulationCommandCallback) {
            simulationCommandCallback(exe.value.first.c_str(), exe.value.second.c_str(), simulationCommandCallbackData);
        } else {
            CERR << "received command, but required callback is not set" << endl;
        }
    } break;
    case InSituMessageType::GoOn: {
        //this is only to get the libsim interface to receive a tcp message and proceed
    } break;
    case InSituMessageType::ConnectionClosed: {
        CERR << "connection closed" << endl;
        return false;
    } break;
    case InSituMessageType::IntOption: {
        auto &option = msg.unpackOrCast<IntOption>().value;
        updateObjectRules(option);
        if (option.name == "keep timesteps") {
            m_processedTimesteps = 0;
            ++m_iterations;
        }
        m_dataTransmitter->resetCache();
        m_dataTransmitter->updateGeneration();
    } break;
    case InSituMessageType::ConnectPort:
    case InSituMessageType::DisconnectPort: {
        m_dataTransmitter->updateRequestedObjets(m_moduleInfo);
    } break;
    default:
        return true;
    }
    return true;
}

void Engine::SetSimulationCommandCallback(void (*sc)(const char *, const char *, void *), void *scdata)
{
    simulationCommandCallback = sc;
    simulationCommandCallbackData = scdata;
}

void Engine::setSlaveComandCallback(void (*sc)(void))
{
    DEBUG_CERR << "setSlaveComandCallback" << endl;
    slaveCommandCallback = sc;
}

message::socket_handle Engine::GetInputSocket()
{
    return m_messageHandler ? m_messageHandler->socketDescriptor() : message::socket_handle{0};
}

//...................................................................................................
//----------------private----------------------------------------------------------------------------
Engine::Engine()
{
    auto comm = MPI_COMM_WORLD;
    setMpiComm(static_cast<void *>(&comm));
#ifndef MODULE_THREAD
    static bool typesREgistered = false;
    if (!typesREgistered) {
        typesREgistered = true;
        vistle::registerTypes();
    }
#endif
}

Engine::~Engine()
{
    m_messageHandler->send(ConnectionClosed{true});
#ifdef MODULE_THREAD
    m_managerThread.join();
#endif
    Engine::instance = nullptr;
}
#ifdef MODULE_THREAD
bool Engine::startVistle(int argC, char **argV)
{
    int prov = MPI_THREAD_SINGLE;
    MPI_Query_thread(&prov);
    if (prov != MPI_THREAD_MULTIPLE) {
        CERR << "startVistle: MPI_THREAD_MULTIPLE not provided" << std::endl;
        return false;
    }
    // comm is not set by the simulation code at
    // this point. Since most simulations never
    // change comm, this is a easy cheat impl
    EngineInterface::initialize(boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate));
    m_modulePort = EngineInterface::getHandler()->port();
    if (connectToVistleModule() && launchManager(argC, argV)) {
        m_initialized = true;
    }
    return m_initialized;
}

bool Engine::launchManager(int argC, char **argV)
{
    const char *VISTLE_ROOT = getenv("VISTLE_ROOT");
    if (!VISTLE_ROOT) {
        CERR << "VISTLE_ROOT not set to the path of the Vistle build directory." << endl;
        return false;
    }

    m_managerThread = std::thread([argC, argV, VISTLE_ROOT]() {
        string cmd{VISTLE_ROOT};
        cmd += "/bin/vistle_manager";
        vector<char *> args;
        args.push_back(const_cast<char *>(cmd.c_str()));
        for (int i = 1; i < argC; ++i) {
            args.push_back(argV[i]);
        }
        vistle::VistleManager manager;
        manager.run(args.size(), args.data());
    });
    return true;
}
#endif

bool Engine::checkInitArgs(int argC, char **argV)
{
    auto numParams = 1;
    if (argC != numParams) {
        CERR << "simulation requires exactly " << numParams << " parameters" << endl;
        return false;
    }
    return true;
}

void Engine::collectModuleInfo(char **argV)
{
    ModuleInfo::ShmInfo shmInfo;
    shmInfo.name = argV[2];
    shmInfo.id = atoi(argV[3]);
    shmInfo.hostname = argV[4];
    shmInfo.numCons = atoi(argV[6]);
    shmInfo.shmName = argV[1];
    m_moduleInfo.update(shmInfo);
    m_modulePort = atoi(argV[5]);
}

bool Engine::connectToVistleModule()
{
    try {
        m_messageHandler = std::make_unique<insitu::message::InSituTcp>(
            boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate), "localhost", m_modulePort);
        m_messageHandler->send(GoOn());
    } catch (const vistle::exception &ex) {
        CERR << "connectToVistleModule: " << ex.what() << endl;
        return false;
    }
    return true;
}

void Engine::updateObjectRules(const IntParam &option)
{
    if (option.name == "combine grids") {
        m_rules.combineGrid = option.value;
    } else if (option.name == "constant grids") {
        m_rules.constGrids = option.value;
    } else if (option.name == "keep timesteps") {
        m_rules.keepTimesteps = option.value;
    } else if (option.name == "VTK variables") {
        m_rules.vtkFormat = option.value;
    } else if (option.name == "frequency") {
        m_rules.frequency = option.value;
    }
}

void Engine::initializeSim()
{
    if (!m_moduleInfo.isInitialized()) {
        try {
            m_metaData.fetchFromSim();
            m_messageHandler->send(SetPorts{m_metaData.getMeshAndFieldNames()});

            auto commands = m_metaData.getRegisteredGenericCommands();
            m_messageHandler->send(SetCommands{commands});
            commands = m_metaData.getRegisteredCustomCommands();
            m_messageHandler->send(SetCustomCommands{commands});

        } catch (const InsituException &ex) {
            CERR << "finalizeInit failed: " << ex.what() << endl;
            return;
        }
        m_moduleInfo.setInitState(true);
    }
}

void Engine::sendObjectsToModule()
{
    try {
        m_dataTransmitter->transferObjectsToVistle(m_processedTimesteps, m_iterations, m_moduleInfo, m_rules);
    } catch (const InsituException &ex) {
        CERR << "transferObjectsToVistle failed: " << ex.what() << endl;
    }
}

void Engine::resetDataTransmitter()
{
    if (m_dataTransmitter) {
        m_dataTransmitter->resetCache();
    } else {
        m_dataTransmitter = std::make_unique<DataTransmitter>(m_metaData, m_moduleInfo, m_rank);
    }
}
