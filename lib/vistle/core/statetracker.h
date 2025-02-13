#ifndef VISTLE_CORE_STATETRACKER_H
#define VISTLE_CORE_STATETRACKER_H

#include <vector>
#include <map>
#include <set>
#include <string>

#include <mutex>
#include <condition_variable>

#include <vistle/util/buffer.h>

#include "export.h"
#include "message.h"
#include "messages.h"
#include "availablemodule.h"

namespace vistle {

class Parameter;
typedef std::set<std::shared_ptr<Parameter>> ParameterSet;
class PortTracker;

class V_COREEXPORT StateObserver {
public:
    StateObserver();
    virtual ~StateObserver();

    virtual void newHub(int hubId, const message::AddHub &hub);
    virtual void deleteHub(int hub);
    virtual void moduleAvailable(const AvailableModule &mod);

    virtual void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName);
    virtual void deleteModule(int moduleId);

    enum ModuleStateBits {
        Unknown = 0,
        Known = 1,
        Initialized = 2,
        Killed = 4,
        Quit = 8,
        Busy = 16,
        Executing = 32,
        Crashed = 64,
    };
    virtual void moduleStateChanged(int moduleId, int stateBits);

    virtual void newParameter(int moduleId, const std::string &parameterName);
    virtual void parameterValueChanged(int moduleId, const std::string &parameterName);
    virtual void parameterChoicesChanged(int moduleId, const std::string &parameterName);
    virtual void deleteParameter(int moduleId, const std::string &parameterName);

    virtual void newPort(int moduleId, const std::string &portName);
    virtual void deletePort(int moduleId, const std::string &portName);

    virtual void newConnection(int fromId, const std::string &fromName, int toId, const std::string &toName);

    virtual void deleteConnection(int fromId, const std::string &fromName, int toId, const std::string &toName);

    virtual void info(const std::string &text, message::SendText::TextType textType, int senderId, int senderRank,
                      message::Type refType, const message::uuid_t &refUuid);
    virtual void itemInfo(const std::string &text, message::ItemInfo::InfoType type, int senderId,
                          const std::string &port);
    //! a module sends a status update
    virtual void status(int id, const std::string &text, message::UpdateStatus::Importance importance);
    //! the overall status has changed
    virtual void updateStatus(int id, const std::string &text, message::UpdateStatus::Importance importance);

    virtual void quitRequested();

    virtual void resetModificationCount();
    virtual void incModificationCount();
    long modificationCount() const;

    virtual void loadedWorkflowChanged(const std::string &filename, int sender);
    virtual void sessionUrlChanged(const std::string &url);

    virtual void message(const vistle::message::Message &msg, vistle::buffer *payload = nullptr);

    virtual void uiLockChanged(bool locked) {}

private:
    long m_modificationCount = 0;
};

struct V_COREEXPORT HubData {
    HubData(int id, const std::string &name);

    int id;
    std::string name;
    std::string logName;
    std::string realName;
    std::string systemType;
    std::string arch;
    std::string info;
    std::string version;
    int numRanks = 0;
    unsigned short port;
    unsigned short dataPort;
    boost::asio::ip::address address;
    bool hasUi = false;
    bool isQuitting = false;
};

class V_COREEXPORT StateTracker {
    friend class ClusterManager;
    friend class Hub;
    friend class DataProxy;
    friend class PortTracker;

public:
    StateTracker(int id, const std::string &name,
                 std::shared_ptr<PortTracker> portTracker = std::shared_ptr<PortTracker>());
    ~StateTracker();

    typedef std::recursive_mutex mutex;
    typedef std::unique_lock<mutex> mutex_locker;
    mutex &getMutex();
    void lock();
    void unlock();
    bool quitting() const;

    void setId(int id);

    bool dispatch(bool &received);

    int getMasterHub() const;
    std::vector<int> getHubs() const;
    std::vector<int> getSlaveHubs() const;
    const std::string &hubName(int id) const;
    std::vector<int> getRunningList() const;
    unsigned getNumRunning() const;
    std::vector<int> getBusyList() const;
    int getHub(int id) const;
    const HubData &getHubData(int id) const;
    std::string getModuleName(int id) const;
    std::string getModuleDescription(int id) const;
    bool isCompound(int id);

    int getModuleState(int id) const;
    int getMirrorId(int id) const;
    const std::set<int> &getMirrors(int id) const;

    std::vector<std::string> getParameters(int id) const;
    std::shared_ptr<Parameter> getParameter(int id, const std::string &name) const;

    ParameterSet getConnectedParameters(const Parameter &param) const;

    bool handle(const message::Message &msg, const buffer *payload, bool track = true);
    bool handle(const message::Message &msg, const char *payload, size_t payloadSize, bool track = true);

    template<typename ConnMsg>
    bool handleConnectOrDisconnect(const ConnMsg &msg)
    {
        return handlePriv(msg);
    }

    std::shared_ptr<PortTracker> portTracker() const;

    struct MessageWithPayload {
        MessageWithPayload(const message::Message &m, std::shared_ptr<const buffer> payload)
        : message(m), payload(payload)
        {}

        message::Buffer message;
        std::shared_ptr<const buffer> payload;
    };
    typedef std::vector<MessageWithPayload> VistleState;

    VistleState getState() const;

    const std::map<AvailableModule::Key, AvailableModule> &availableModules() const;

    void registerObserver(StateObserver *observer) const;
    void unregisterObserver(StateObserver *observer) const;

    bool registerRequest(const message::uuid_t &uuid);
    std::shared_ptr<message::Buffer> waitForReply(const message::uuid_t &uuid);

    std::vector<int> waitForSlaveHubs(size_t count);
    std::vector<int> waitForSlaveHubs(const std::vector<std::string> &names);

    int graphChangeCount() const;

    std::string loadedWorkflowFile() const;
    int workflowLoader() const;
    std::string sessionUrl() const;

    void printModules(bool withConnections = false) const;

    enum ConnectionKind {
        Neighbor,
        Upstream,
        Downstream,
        Previous,
        Next,
    };

    std::set<int> getConnectedModules(ConnectionKind kind, int id, const std::string &port = std::string()) const;

    std::string barrierInfo(const message::uuid_t &uuid) const;

    void setVerbose(bool verbose);

protected:
    std::shared_ptr<message::Buffer> removeRequest(const message::uuid_t &uuid);
    bool registerReply(const message::uuid_t &uuid, const message::Message &msg);

    typedef std::map<std::string, std::shared_ptr<Parameter>> ParameterMap;
    typedef std::map<int, std::string> ParameterOrder;
    struct Module {
        int id;
        int mirrorOfId = message::Id::Invalid;
        std::set<int> mirrors;
        int hub;
        unsigned long rank0Pid = 0;
        bool initialized = false;
        bool killed = false;
        bool busy = false;
        bool executing = false;
        bool crashed = false;
        bool outputStreaming = false;
        std::string name;
        ParameterMap parameters;
        ParameterOrder paramOrder;
        int height = 0; //< length of shortest path to a sink
        std::string statusText;
        message::UpdateStatus::Importance statusImportance = message::UpdateStatus::Bulk;
        unsigned long statusTime = 0;

        message::ObjectReceivePolicy::Policy objectPolicy;
        message::SchedulingPolicy::Schedule schedulingPolicy;
        message::ReducePolicy::Reduce reducePolicy;

        int state() const;
        bool isSink() const { return height == 0; }
        bool isExecuting() const { return executing; }
        Module(int id, int hub)
        : id(id)
        , hub(hub)
        , objectPolicy(message::ObjectReceivePolicy::Local)
        , schedulingPolicy(message::SchedulingPolicy::Single)
        , reducePolicy(message::ReducePolicy::Locally)
        {}
    };

    typedef std::map<int, Module> RunningMap;
    RunningMap runningMap; //< currently running modules on all connected clusters
    void computeHeights(); //< compute heights for all modules in runningMap
    RunningMap quitMap; //< history of already terminated modules - for module -> hub mapping
    typedef std::set<int> ModuleSet;
    ModuleSet busySet;
    int m_graphChangeCount = 0;
    std::set<int> getUpstreamModules(int id, const std::string &port = std::string(), bool recurse = true) const;
    std::set<int> getDownstreamModules(int id, const std::string &port = std::string(), bool recurse = true,
                                       bool ignoreNoCompute = false) const;
    std::set<int> getDownstreamModules(const message::Execute &exec) const;
    bool hasCombinePort(int id) const;
    bool isExecuting(int id) const;

    void appendModuleState(VistleState &state, const Module &mod) const;
    void appendModuleParameter(VistleState &state, const Module &mod) const;
    void appendModulePorts(VistleState &state, const Module &mod) const;
    void appendModuleOutputConnections(VistleState &state, const Module &mod) const;


    std::map<AvailableModule::Key, AvailableModule> m_availableModules;

    mutable std::set<StateObserver *> m_observers;

    VistleState m_queue;
    void processQueue();
    void cleanQueue(int moduleId);
    bool m_processingQueue = false;

    std::string statusText() const;

private:
    void updateStatus();

    bool handlePriv(const message::AddHub &slave);
    bool handlePriv(const message::RemoveHub &slave);
    bool handlePriv(const message::Trace &trace);
    bool handlePriv(const message::Debug &debug);
    bool handlePriv(const message::Spawn &spawn);
    bool handlePriv(const message::LoadWorkflow &load);
    bool handlePriv(const message::SaveWorkflow &save);
    bool handlePriv(const message::Started &started);
    bool handlePriv(const message::Connect &connect);
    bool handlePriv(const message::Disconnect &disc);
    bool handlePriv(const message::ModuleExit &moduleExit);
    bool handlePriv(const message::Execute &execute);
    bool handlePriv(const message::ExecutionProgress &prog);
    bool handlePriv(const message::ExecutionDone &done);
    bool handlePriv(const message::Busy &busy);
    bool handlePriv(const message::Idle &idle);
    bool handlePriv(const message::AddPort &createPort);
    bool handlePriv(const message::RemovePort &destroyPort);
    bool handlePriv(const message::AddParameter &addParam);
    bool handlePriv(const message::RemoveParameter &removeParam);
    bool handlePriv(const message::SetParameter &setParam);
    bool handlePriv(const message::SetParameterChoices &choices, const buffer &payload);
    bool handlePriv(const message::Kill &kill);
    bool handlePriv(const message::AddObject &addObj);
    bool handlePriv(const message::Barrier &barrier);
    bool handlePriv(const message::BarrierReached &barrierReached);
    bool handlePriv(const message::SendText &info, const buffer &payload);
    bool handlePriv(const message::ItemInfo &info, const buffer &payload);
    bool handlePriv(const message::UpdateStatus &status);
    bool handlePriv(const message::ReplayFinished &reset);
    bool handlePriv(const message::Quit &quit);
    bool handlePriv(const message::ModuleAvailable &mod, const buffer &payload);
    bool handlePriv(const message::ObjectReceivePolicy &pol);
    bool handlePriv(const message::ReducePolicy &pol);
    bool handlePriv(const message::SchedulingPolicy &pol);
    bool handlePriv(const message::RequestTunnel &tunnel);
    bool handlePriv(const message::CloseConnection &close);

    HubData *getModifiableHubData(int id);

    int m_id = message::Id::Invalid;
    std::shared_ptr<PortTracker> m_portTracker;

    std::set<message::uuid_t> m_alreadySeen;

    mutex m_replyMutex;
    std::condition_variable_any m_replyCondition;
    std::map<message::uuid_t, std::shared_ptr<message::Buffer>> m_outstandingReplies;

    mutex m_slaveMutex;
    std::condition_variable_any m_slaveCondition;

    message::Type m_traceType;
    int m_traceId;
    std::string m_name;
    std::vector<HubData> m_hubs;

    unsigned long m_statusTime = 1;
    int m_currentStatusId = message::Id::Invalid;
    std::string m_currentStatus;
    message::UpdateStatus::Importance m_currentStatusImportance = message::UpdateStatus::Bulk;

    size_t m_numMessages = 0;
    size_t m_numObjects = 0;
    size_t m_aggregatedPayload = 0;

    mutable mutex m_stateMutex;

    int m_workflowLoader = message::Id::Invalid;
    std::string m_loadedWorkflowFile;
    std::string m_sessionUrl;

    std::map<message::uuid_t, std::string> m_barriers;
    bool m_quitting = false;

    bool m_verbose = false;

    void setModified(const std::string &reason = {});
};

} // namespace vistle

#endif
