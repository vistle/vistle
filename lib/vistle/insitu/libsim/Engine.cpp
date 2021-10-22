#include "Engine.h"

#include "Exception.h"
#include "VertexTypesToVistle.h"
#include "VisitDataTypesToVistle.h"

#include <vistle/insitu/libsim/libsimInterface/SimulationMetaData.h>
#include <vistle/module/module.h>

#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/tcpmessage.h>

#include <vistle/util/directory.h>
#include <vistle/util/hostname.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/sleep.h>
#include <vistle/util/stopwatch.h>
#include <vistle/util/shmconfig.h>

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
    instance->m_messageHandler.send(ConnectionClosed{true});
    delete instance;
    instance = nullptr;
#ifndef MODULE_THREAD
    if (vistle::Shm::isAttached()) {
        vistle::Shm::the().detach();
    }
#endif // !#ifndef MODULE_THREAD
}

bool Engine::initialize(int argC, char **argV)
{
    if (m_initialized) {
        CERR << "initialize called, but engine is already initialized" << endl;
        return false;
    }

    CERR << "__________Engine args__________" << endl;
    for (int i = 0; i < argC; i++) {
        CERR << argV[i] << endl;
    }
    CERR << "_______________________________" << endl;
#ifdef MODULE_THREAD
    return startVistle(argC, argV);
#else
    if (checkInitArgs(argC, argV)) {
        collectModuleInfo(argV);
        return initializeVistleEnv();
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

    static int counter = 0;
    DEBUG_CERR << "SimulationTimeStepChanged counter = " << counter << endl;
    ++counter;
    m_metaData.fetchFromSim();
    if (m_processedCycles > 0 && m_processedCycles >= m_metaData.currentCycle()) {
        DEBUG_CERR << "There is no new timestep but SimulationTimeStepChanged was called" << endl;
        return;
    }
    int numMeshes, numVars;
    v2check(simv2_SimulationMetaData_getNumMeshes, m_metaData.handle(), numMeshes);
    v2check(simv2_SimulationMetaData_getNumVariables, m_metaData.handle(), numVars);
    if (m_metaData.currentCycle() % m_intOptions[IntOptions::NthTimestep].value()) {
        return;
    }
    static std::unique_ptr<StopWatch> watch;
    watch = std::make_unique<StopWatch>(("Engine: [" + std::to_string(m_rank) + "/" + std::to_string(m_mpiSize) +
                                         "]: time since last processed timestep")
                                            .c_str());
    DEBUG_CERR << "Timestep " << m_metaData.currentCycle() << " has " << numMeshes << " meshes and " << numVars
               << "variables" << endl;
    if (m_moduleInfo.isReady()) { // only here vistle::objects are allowed to be made
        sendObjectsToModule();

        m_processedCycles = m_metaData.currentCycle();
        ++m_timestep;
    } else {
        CERR << "ConnectLibSim is not ready to process data" << endl;
    }
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
#ifndef MODULE_THREAD
        m_messageHandler.send(GoOn{}); // request tcp message from controller
#else
        if (EngineInterface::getControllSocket())
            message::send(GoOn{}, *EngineInterface::getControllSocket()); //directly send GoOn to the sim
#endif
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
    static int counter = 0;
    DEBUG_CERR << "handleVistleMessage counter = " << counter << endl;
    ++counter;
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
    auto msg = m_messageHandler.recv();
    DEBUG_CERR << "received message of type " << static_cast<int>(msg.type()) << endl;
    m_moduleInfo.update(msg);
    switch (msg.type()) {
    case InSituMessageType::Ready: {
        if (m_moduleInfo.isReady()) {
            vistle::Shm::the().setObjectID(m_shmIDs.objectID());
            vistle::Shm::the().setArrayID(m_shmIDs.arrayID());
            m_timestep = 0;
        } else {
            m_shmIDs.set(vistle::Shm::the().objectID(),
                         vistle::Shm::the().arrayID()); // send final state just to be safe
            m_messageHandler.send(Ready{false}); // confirm that we are done creating vistle objects
        }
    } break;
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
        m_shmIDs.close();
        return false;
    } break;
    case InSituMessageType::LibSimIntOption: {
        auto &option = msg.unpackOrCast<LibSimIntOption>().value;
        update(m_intOptions, option);

    } break;
#ifdef MODULE_THREAD
    case InSituMessageType::ModuleID: {
        m_moduleInfo.setReadyState(false);
        m_shmIDs.initialize(m_moduleInfo.id(), m_rank, m_moduleInfo.uniqueSuffix(), SyncShmIDs::Mode::Attach);
        resetDataTransmitter();

    } break;
#endif
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

int Engine::GetInputSocket()
{
    if (m_rank == 0 && m_socket) {
        return m_socket->native_handle();
    } else {
        return 0;
    }
}

//...................................................................................................
//----------------private----------------------------------------------------------------------------
Engine::Engine()
{
    m_intOptions[IntOptions::CombineGrids] = IntOption{IntOptions::CombineGrids, false};
    m_intOptions[IntOptions::ConstGrids] = IntOption{IntOptions::ConstGrids, false};
    m_intOptions[IntOptions::KeepTimesteps] = IntOption{IntOptions::KeepTimesteps, 1};
    m_intOptions[IntOptions::NthTimestep] = IntOption{IntOptions::NthTimestep, 1};
    m_intOptions[IntOptions::VtkFormat] = IntOption{IntOptions::VtkFormat, false, [this]() {
                                                        resetDataTransmitter();
                                                    }};

    auto comm = MPI_COMM_WORLD;
    setMpiComm(static_cast<void *>(&comm));
}

Engine::~Engine()
{
    m_messageHandler.send(ConnectionClosed{true});
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
    if ((m_rank == 0 && ConnectMySelf()) || m_rank > 0) {
        m_messageHandler.initialize(
            m_socket,
            boost::mpi::communicator(
                comm, boost::mpi::comm_create_kind::comm_duplicate)); // comm is not set by the simulation code at
        // this point. Since most simulations never
        // change comm, this is a easy cheat impl
        if (launchManager(argC, argV)) {
            m_initialized = true;
        }
    }
    return m_initialized;
}

bool Engine::ConnectMySelf()
{
    initializeAsync();
    unsigned short port = m_port;

    boost::system::error_code ec;

    while (!vistle::start_listen(port, *m_acceptorv4, *m_acceptorv6, ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else if (ec) {
            CERR << "failed to listen on port " << port << ": " << ec.message() << endl;
            return false;
        }
    }
    m_port = port;
    startAccept(m_acceptorv4);
    startAccept(m_acceptorv6);
    connectToModule("localhost", m_port);
    return true;
}

void Engine::initializeAsync()
{
#if BOOST_VERSION >= 106600
    m_workGuard = std::make_unique<WorkGuard>(m_ioService.get_executor());
#else
    m_workGuard = std::unique_ptr<WorkGuard>(new boost::asio::io_service::work(m_ioService));
#endif

    m_ioThread = std::make_unique<std::thread>([this]() {
        m_ioService.run();
        DEBUG_CERR << "io thread terminated" << endl;
    });

    m_acceptorv4.reset(new acceptor(m_ioService));
    m_acceptorv6.reset(new acceptor(m_ioService));
}

bool Engine::startAccept(std::unique_ptr<acceptor> &a)
{
    if (!a->is_open())
        return false;

    std::shared_ptr<socket> sock(new socket(m_ioService));
    boost::system::error_code ec;
    a->async_accept(*sock, [this, &a, sock](boost::system::error_code ec) {
        if (ec) {
            if (m_waitingForAccept) {
                EngineInterface::setControllSocket(nullptr);
                CERR << "failed connection attempt" << endl;
            }
            return;
        }
        CERR << "server connected with engine, acceptor " << (a == m_acceptorv4 ? "v4" : "v6") << endl;
        EngineInterface::setControllSocket(sock);
        m_waitingForAccept = false;
        a == m_acceptorv4 ? m_acceptorv6->close() : m_acceptorv4->close();
    });
    return true;
}

bool Engine::launchManager(int argC, char **argV)
{
    const char *VISTLE_ROOT = getenv("VISTLE_ROOT");
    if (!VISTLE_ROOT) {
        CERR << "VISTLE_ROOT not set to the path of the Vistle build directory." << endl;
        return false;
    }

    m_managerThread = std::thread([this, argC, argV, VISTLE_ROOT]() {
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
    if (argC != 7) {
        CERR << "simulation requires exactly 7 parameters" << endl;
        return false;
    }
    if (atoi(argV[0]) != m_mpiSize) {
        CERR << "mpi size of simulation must match vistle's mpi size" << endl;
        return false;
    }
    if (m_rank == 0 && argV[4] != vistle::hostname()) {
        CERR << "this " << vistle::hostname() << "trying to connect to " << argV[4] << endl;
        CERR << "Wrong host: must connect to Vistle on the same machine!" << endl;
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

bool Engine::initializeVistleEnv()
{
    vistle::registerTypes();
    try {
        bool shmPerRank = vistle::shmPerRank();
        auto shmName = m_moduleInfo.shmName();
        CERR << "attaching to shm: name = " << shmName << " id = " << m_moduleInfo.id() << " rank = " << m_rank << endl;
        vistle::Shm::attach(shmName, m_moduleInfo.id(), m_rank, shmPerRank);
        if (m_rank == 0) {
            connectToModule(m_moduleInfo.hostname(), m_modulePort);
        }
        m_shmIDs.initialize(m_moduleInfo.id(), m_rank, m_moduleInfo.uniqueSuffix(), SyncShmIDs::Mode::Attach);
        resetDataTransmitter();
        m_messageHandler.initialize(m_socket,
                                    boost::mpi::communicator(comm, boost::mpi::comm_create_kind::comm_duplicate));
        m_messageHandler.send(GoOn());
    } catch (const vistle::exception &ex) {
        CERR << "initializeVistleEnv: " << ex.what() << endl;
        return false;
    }

    m_initialized = true;
    return true;
}

Rules Engine::gatherObjectRules()
{
    Rules rules;
    rules.combineGrid = m_intOptions[IntOptions::CombineGrids].value();
    rules.constGrids = m_intOptions[IntOptions::ConstGrids].value();
    rules.keepTimesteps = m_intOptions[IntOptions::KeepTimesteps].value();
    rules.vtkFormat = m_intOptions[IntOptions::VtkFormat].value();
    return rules;
}

void Engine::connectToModule(const string &hostname, int port)
{
    boost::system::error_code ec;
    asio::ip::tcp::resolver resolver(m_ioService);
    asio::ip::tcp::resolver::query query(hostname, boost::lexical_cast<string>(port));
    m_socket.reset(new socket(m_ioService));
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
    if (ec) {
        throw EngineException("initializeEngineSocket failed to resolve connect socket");
    }

    asio::connect(*m_socket, endpoint_iterator, ec);

    if (ec) {
        throw EngineException("initializeEngineSocket failed to connect socket");
    }
}

void Engine::initializeSim()
{
    if (!m_moduleInfo.isInitialized()) {
        try {
            m_metaData.fetchFromSim();
            m_messageHandler.send(SetPorts{m_metaData.getMeshAndFieldNames()});

            auto commands = m_metaData.getRegisteredGenericCommands();
            m_messageHandler.send(SetCommands{commands});
            commands = m_metaData.getRegisteredCustomCommands();
            m_messageHandler.send(SetCustomCommands{commands});

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
        m_dataTransmitter->transferObjectsToVistle(m_timestep, m_moduleInfo, gatherObjectRules());
    } catch (const InsituException &ex) {
        CERR << "transferObjectsToVistle failed: " << ex.what() << endl;
    }
}

void Engine::resetDataTransmitter()
{
    if (m_dataTransmitter) {
        m_dataTransmitter->resetCache();
    } else {
        m_dataTransmitter = std::make_unique<DataTransmitter>(m_metaData, m_shmIDs, m_moduleInfo, m_rank);
    }
}
