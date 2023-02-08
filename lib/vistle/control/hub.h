#ifndef VISTLE_HUB_H
#define VISTLE_HUB_H

#include <memory>
#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <boost/process.hpp>
#include <vistle/core/statetracker.h>
#include <vistle/util/buffer.h>
#include "uimanager.h"
#include <vistle/net/tunnel.h>
#include <vistle/net/dataproxy.h>
#include <vistle/control/scanmodules.h>
#include <vistle/core/parametermanager.h>

#include "export.h"

namespace vistle {

namespace config {
class Access;
}

class PythonInterpreter;
class Directory;
class Hub;
class HubParameters: public ParameterManager {
public:
    HubParameters(Hub &hub);
    void sendParameterMessage(const message::Message &message, const buffer *payload = nullptr) const override;

private:
    Hub &m_hub;
};

class ConfigParameters: public ParameterManager {
public:
    ConfigParameters(Hub &hub);
    void sendParameterMessage(const message::Message &message, const buffer *payload = nullptr) const override;

private:
    Hub &m_hub;
};

class V_HUBEXPORT Hub {
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef std::shared_ptr<socket> socket_ptr;

    static Hub &the();

    Hub(bool inManager = false);
    ~Hub();

    int run();

    bool init(int argc, char *argv[]);
    bool processScript();
    bool dispatch();
    bool sendMessage(socket_ptr sock, const message::Message &msg, const buffer *payload = nullptr);
    unsigned short port() const;
    unsigned short dataPort() const;
    std::shared_ptr<boost::process::child> launchProcess(const std::string &prog, const std::vector<std::string> &argv);
    std::shared_ptr<boost::process::child> launchMpiProcess(const std::vector<std::string> &argv);
    const std::string &name() const;

    bool handleMessage(const message::Message &msg, socket_ptr sock = socket_ptr(), const buffer *payload = nullptr);
    bool sendManager(const message::Message &msg, int hub = message::Id::LocalHub, const buffer *payload = nullptr);
    bool sendMaster(const message::Message &msg, const buffer *payload = nullptr);
    bool sendSlaves(const message::Message &msg, bool returnToSender = false, const buffer *payload = nullptr);
    bool sendHub(int id, const message::Message &msg, const buffer *payload = nullptr);
    bool sendUi(const message::Message &msg, int id = message::Id::Broadcast, const buffer *payload = nullptr);
    bool sendModule(const message::Message &msg, int id, const buffer *payload = nullptr);
    bool sendAll(const message::Message &msg, const buffer *payload = nullptr);
    bool sendAllUi(const message::Message &msg, const buffer *payload = nullptr);
    bool sendAllButUi(const message::Message &msg, const buffer *payload = nullptr);

    const StateTracker &stateTracker() const;
    StateTracker &stateTracker();

    int idToHub(int id) const;
    int id() const;

private:
    struct Slave;
    std::unique_ptr<config::Access> m_config;

    message::MessageFactory make;

    bool hubReady();
    bool connectToMaster(const std::string &host, unsigned short port);
    bool startVrb();
    void stopVrb();
    socket_ptr connectToVrb(unsigned short port);
    bool handleVrb(socket_ptr sock);
    bool startUi(const std::string &uipath, bool replace = false);
    bool startPythonUi();
    bool startServer();
    bool startAccept(std::shared_ptr<acceptor> a);
    void handleWrite(socket_ptr sock, const boost::system::error_code &error);
    void handleAccept(std::shared_ptr<acceptor> a, socket_ptr sock, const boost::system::error_code &error);
    void addSocket(socket_ptr sock, message::Identify::Identity ident = message::Identify::UNKNOWN);
    bool removeSocket(socket_ptr sock, bool close = true);
    void addClient(socket_ptr sock);
    void addSlave(const std::string &name, socket_ptr sock);
    void removeSlave(int id);
    bool removeClient(socket_ptr sock);
    void slaveReady(Slave &slave);
    bool startCleaner();
    bool processScript(const std::string &filename, bool barrierAfterLoad, bool executeModules);
    bool processStartupScripts();
    bool processCommand(const std::string &filename);
    bool cacheModuleValues(int oldModuleId, int newModuleId);
    void killOldModule(int migratedId);
    void sendInfo(const std::string &s);
    void sendError(const std::string &s);
    std::vector<int> getSubmoduleIds(int modId, const AvailableModule &av);
    bool m_inManager = false;
    bool m_proxyOnly = false;
    vistle::message::AddHub addHubForSelf() const;

    unsigned short m_basePort = 31093;
    unsigned short m_port = 0, m_dataPort = 0, m_masterPort = m_basePort;
    std::string m_exposedHost;
    boost::asio::ip::address m_exposedHostAddr;
    std::string m_masterHost;
    std::string m_conferenceUrl;
    std::string m_sessionUrl;
    boost::asio::io_service m_ioService;
    std::shared_ptr<acceptor> m_acceptorv4, m_acceptorv6;

    std::mutex m_socketMutex; // protect access to m_sockets and m_clients
    std::map<socket_ptr, message::Identify::Identity> m_sockets;
    std::set<socket_ptr> m_clients;

    unsigned short m_vrbPort = 0;
    std::map<int, socket_ptr> m_vrbSockets;
    std::shared_ptr<boost::process::child> m_vrb;

    std::shared_ptr<DataProxy> m_dataProxy;
    TunnelManager m_tunnelManager;
    StateTracker m_stateTracker;
    UiManager m_uiManager;
    bool m_hasUi = false;
    bool m_hasVrb = false;

    std::mutex m_processMutex; // protect access to m_processMap
    std::map<std::shared_ptr<boost::process::child>, int> m_processMap;
    bool m_managerConnected;

    std::unique_ptr<vistle::Directory> m_dir;
    std::string m_scriptPath;
    std::string m_snapshotFile;
    bool m_barrierAfterLoad = true;
    bool m_executeModules = false;
    bool m_quitting = false, m_emergency = false;
    int m_numRunningModules = 0;
    std::function<bool(void)> m_lastModuleQuitAction;
    static volatile std::atomic<bool> m_interrupt;
    boost::asio::signal_set m_signals;
    static void signalHandler(const boost::system::error_code &error, int signal_number);

    bool m_isMaster;
    socket_ptr m_masterSocket;
    struct Slave {
        socket_ptr sock;
        std::string name;
        bool ready = false;
        int id = 0;
    };
    std::map<int, Slave> m_slaves;
    std::vector<Slave *> m_slavesToConnect;
    int m_slaveCount = 0;
    int m_hubId = vistle::message::Id::Invalid;
    int m_localRanks = -1;
    unsigned long m_localManagerRank0Pid = 0;
    std::string m_name;
    bool m_ready = false;

    int m_moduleCount;
    message::Type m_traceMessages;

    int m_execCount;

    bool m_barrierActive;
    unsigned m_barrierReached;
    message::uuid_t m_barrierUuid;

    std::string m_statusText;

    bool handlePriv(const message::Quit &quit, message::Identify::Identity senderType);
    bool handlePriv(const message::RemoveHub &rm);
    bool handlePriv(const message::Execute &exec);
    bool handlePriv(const message::ExecutionDone &done);
    bool handlePriv(const message::CancelExecute &cancel);
    bool handlePriv(const message::Barrier &barrier);
    bool handlePriv(const message::BarrierReached &reached);
    bool handlePriv(const message::RequestTunnel &tunnel);
    bool handlePriv(const message::Connect &conn);
    bool handlePriv(const message::Disconnect &disc);
    bool handlePriv(const message::FileQuery &query, const buffer *payload);
    bool handlePriv(const message::FileQueryResult &result, const buffer *payload);
    bool handlePriv(const message::Cover &cover, const buffer *payload);
    bool handlePriv(const message::ModuleExit &exit);
    bool handlePriv(const message::Spawn &spawn);
    bool handlePriv(const message::LoadWorkflow &load);
    bool handlePriv(const message::SaveWorkflow &save);

    template<typename ConnMsg>
    bool handleConnectOrDisconnect(const ConnMsg &mm);

    bool checkChildProcesses(bool emergency = false, bool onMainThread = true);
    bool hasChildProcesses(bool ignoreGui = false);
    void emergencyQuit();
    const AvailableModule *findModule(const AvailableModule::Key &key);
    void spawnModule(const std::string &path, const std::string &name, int spawnId);
    bool spawnMirror(int hubId, const std::string &name, int mirroredId);
    std::vector<message::Buffer> m_queue;
    bool handleQueue();

    void setLoadedFile(const std::string &file);
    void setSessionUrl(const std::string &url);
    void setStatus(const std::string &s, message::UpdateStatus::Importance prio = message::UpdateStatus::Low);
    void clearStatus();
    void updateLinkedParameters(const message::SetParameter &setParam);
    std::map<int, std::vector<message::Buffer>> m_sendAfterExit, m_sendAfterSpawn;

#if BOOST_VERSION >= 106600
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
#else
    std::shared_ptr<boost::asio::io_service::work> m_workGuard;
#endif
    std::vector<std::thread> m_ioThreads;
    std::vector<std::thread> m_vrbThreads;
    void startIoThread();
    void stopIoThreads();

    HubParameters params;
    ConfigParameters settings;

    std::mutex m_outstandingDataConnectionMutex;
    std::map<vistle::message::AddHub, std::future<bool>> m_outstandingDataConnections;

    std::unique_ptr<PythonInterpreter> m_python;
};

} // namespace vistle
#endif
