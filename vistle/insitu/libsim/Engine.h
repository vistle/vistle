#ifndef VISIT_VISTLE_ENGINE_H
#define VISIT_VISTLE_ENGINE_H

#include "DataTransmitter.h"
#include "IntOption.h"
#include "MeshInfo.h"
#include "MetaData.h"
#include "export.h"

#include <vistle/insitu/message/SyncShmIDs.h>
#include <vistle/insitu/message/TcpMessage.h>
#include <vistle/insitu/message/addObjectMsq.h>
#include <vistle/util/enumarray.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <mpi.h>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace vistle {
namespace message {
class MessageQueue;
}
namespace insitu {
namespace libsim {

#ifdef MODULE_THREAD
class V_VISITXPORT Engine {
#else
class V_VISITXPORT Engine {
#endif
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    static Engine *EngineInstance();
    static void DisconnectSimulation();
    bool initialize(int argC, char **argV);
    bool isInitialized() const noexcept;
    bool setMpiComm(void *newConn);

    //********************************
    //****functions called by sim****
    //********************************
    // does nothing, should add all available data to the according outputs to execute the pipeline
    bool sendData();
    // called from simulation when a timestep changed
    void SimulationTimeStepChanged();
    // called by the static LibSim library for syncing. So far only handles "INTERNALSYNC" commands.
    // When this function gets called, the simulationCommandCallback has been replaced with and internal callback for
    // the duration of the call.
    void SimulationInitiateCommand(const std::string &command);
    // not sure what to do here.
    void DeleteData();
    // called by the sim on rank 0 when m_socket receives data and on other ranks when we call slaveCommandCallback.
    // To distribude commands on all ranks, rank 0 calls slaveCommandCallback, reads from socked and broadcasts
    // while the other ranks only do the broadcast.
    // then handles the received Vistle message
    bool fetchNewModuleState();

    // set callbacks (from sim)
    void SetSimulationCommandCallback(void (*sc)(const char *, const char *, void *), void *scdata);
    // set callbacks (called from sim and from the LibSim static library whyle syncing)
    void setSlaveComandCallback(void (*sc)(void));
    // return the file descripter of m_socket so that LibSim can wait for messages on that socket
    int GetInputSocket();

private:
    static Engine *instance;
    bool m_initialized = false; // Engine is initialized
    // mpi info
    int m_rank = -1, m_mpiSize = 0;
    MPI_Comm comm = MPI_COMM_WORLD;

    insitu::message::InSituTcp m_messageHandler;
    insitu::message::SyncShmIDs m_shmIDs;

    // Port info to communicate with the vistle module
    unsigned short m_port = 31099;
    boost::asio::io_service m_ioService;

    std::shared_ptr<socket> m_socket;
    // info from the simulation
    size_t m_processedCycles = 0; // the last cycle that was processed
    MetaData m_metaData; // the meta data of the currenc cycle
    std::unique_ptr<DataTransmitter> m_dataTransmitter;

    message::ModuleInfo m_moduleInfo;
    int m_modulePort = 0;
    size_t m_timestep = 0; // timestep counter for module

    //std::set<IntOption> m_intOptions; // options that can be set in the module
    vistle::EnumArray<IntOption, IntOptions> m_intOptions;
    // callbacks from simulation
    void (*simulationCommandCallback)(const char *, const char *, void *) = nullptr;
    void *simulationCommandCallbackData = nullptr;
    void (*slaveCommandCallback)(void) = nullptr;
#ifdef MODULE_THREAD
    std::thread m_managerThread;
#if BOOST_VERSION >= 106600
    typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> WorkGuard;
#else
    typedef boost::asio::io_service::work WorkGuard;
#endif
    std::unique_ptr<WorkGuard> m_workGuard;
    std::unique_ptr<std::thread> m_ioThread; // thread for io_service
    std::unique_ptr<acceptor> m_acceptorv4, m_acceptorv6;
    std::mutex m_asioMutex;
    std::atomic<bool> m_waitingForAccept{true}; // condition
#endif
    Engine();
    ~Engine();
#ifdef MODULE_THREAD
    bool startVistle(int argC, char **argV);
    bool ConnectMySelf();
    void initializeAsync();
    bool startAccept(std::unique_ptr<acceptor> &a);
    bool launchManager(int argC, char **argV);
#endif // MODULE_THREAD

    bool checkInitArgs(int argC, char **argV);
    void collectModuleInfo(char **argV);
    bool initializeVistleEnv();
    Rules gatherObjectRules();
    void connectToModule(const std::string &hostname, int port);
    void initializeSim(); // if not already done, initializes the things that require the simulation
    void sendObjectsToModule();
    void resetDataTransmitter();
};

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !VISIT_VISTLE_ENGINE_H
