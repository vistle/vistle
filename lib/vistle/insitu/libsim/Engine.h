#ifndef VISIT_VISTLE_ENGINE_H
#define VISIT_VISTLE_ENGINE_H

#include "DataTransmitter.h"
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

    std::unique_ptr<insitu::message::InSituTcp> m_messageHandler; //handles the communication with the module
    //(the initial connection is established via different sockets)

    // Port info to communicate with the vistle module
    unsigned short m_port = 31099;

    // info from the simulation
    MetaData m_metaData; // the meta data of the currenc cycle
    std::unique_ptr<DataTransmitter> m_dataTransmitter;

    message::ModuleInfo m_moduleInfo; //information about the connected module
    int m_modulePort = 0; //port to connect to the module (hostname is localhost)
    size_t m_processedTimesteps = 0; // timestep counter for module
    size_t m_iterations = 0; //iteration if we do not keep timesteps
    Rules m_rules; //the options configured in the connected module
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
    bool connectToVistleModule();
    int getIntParam(const std::string &name);

    void updateObjectRules(const message::IntParam &option); //translate the intOptions in rules to create vistle data
    void connectToModule(const std::string &hostname, int port);
    void initializeSim(); // if not already done, initializes the things that require the simulation
    void sendObjectsToModule();
    void resetDataTransmitter();
};

} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !VISIT_VISTLE_ENGINE_H
