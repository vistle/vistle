#ifndef CLUSTERMANAGER_H
#define CLUSTERMANAGER_H

#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include <vistle/core/archives_config.h>
#include <vistle/core/message.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/parameter.h>
#include <vistle/core/parametermanager.h>
#include <vistle/core/statetracker.h>
#include <vistle/util/directory.h>
#include <vistle/util/enum.h>

#include "portmanager.h"

#include <boost/mpi.hpp>

#ifdef MODULE_THREAD
#include <vistle/module/module.h>
#include <boost/function.hpp>
#endif


namespace vistle {

namespace message {
class Message;
class MessageQueue;
} // namespace message

class Parameter;

class ClusterManager {
    friend class Communicator;
    friend class DataManager;

public:
    ClusterManager(boost::mpi::communicator comm, const std::vector<std::string> &hosts);
    virtual ~ClusterManager();

    void init(); // to be called when message sending is possible

    bool dispatch(bool &received);
    const StateTracker &state() const;

    bool sendMessage(int receiver, const message::Message &message, int destRank = -1,
                     const MessagePayload &payload = MessagePayload()) const;
    bool sendAll(const message::Message &message, const MessagePayload &payload = MessagePayload()) const;
    bool sendAllLocal(const message::Message &message, const MessagePayload &payload = MessagePayload()) const;
    bool sendAllOthers(int excluded, const message::Message &message, const MessagePayload &payload,
                       bool localOnly = false) const;
    bool sendUi(const message::Message &message, const MessagePayload &paylod = MessagePayload()) const;
    bool sendHub(const message::Message &message, const MessagePayload &payload = MessagePayload(),
                 int destHub = message::Id::Broadcast) const;

    bool quit();
    bool quitOk() const;

    int getRank() const;
    int getSize() const;

    std::string getModuleName(int id) const;
    std::vector<std::string> getParameters(int id) const;
    std::shared_ptr<Parameter> getParameter(int id, const std::string &name) const;

    PortManager &portManager() const;

    bool handle(const message::Buffer &msg, const MessagePayload &payload = MessagePayload());
    //bool handleData(const message::Message &msg);

    message::CompressionMode archiveCompressionMode() const;
    int archiveCompressionSpeed() const;
    const CompressionSettings &compressionSettings();

    bool isLocal(int id) const;
    int idToHub(int id) const;
    int hubId() const;

private:
    void queueMessage(const message::Message &msg);
    void replayMessages();
    std::vector<message::Buffer> m_messageQueue;

    boost::mpi::communicator m_comm;
    std::shared_ptr<PortManager> m_portManager;
    StateTracker m_stateTracker;
    message::Type m_traceMessages;

    AvailableMap m_localModules;

    struct CompModuleHeight {
        bool operator()(const StateTracker::Module &a, const StateTracker::Module &b) { return a.height > b.height; }
    };
    std::vector<StateTracker::Module> m_modulePriority;
    int m_modulePriorityChange = -1;

    bool m_quitFlag;

    struct PortKey {
        PortKey(int module, const std::string &port): module(module), port(port) {}
        PortKey(const Port *p): module(p->getModuleID()), port(p->getName()) {}

        bool operator<(const PortKey &other) const
        {
            if (module == other.module)
                return port < other.port;
            return module < other.module;
        }

        int module;
        std::string port;
    };
    struct PortObjectCache {
        int iteration = 0;
        int execCount = 0;
        std::vector<std::string> objects;
    };
    std::map<PortKey, PortObjectCache> m_outputObjects; // current objects at local output ports
    bool addObjectSource(const message::AddObject &addObj);
    bool addObjectDestination(const message::AddObject &addObj, Object::const_ptr obj);

    bool handlePriv(const message::Trace &trace);
    bool handlePriv(const message::Quit &quit);
    bool handlePriv(const message::Spawn &spawn);
    bool handlePriv(const message::Connect &connect);
    bool handlePriv(const message::Disconnect &disc);
    bool handlePriv(const message::ModuleExit &moduleExit);
    bool handlePriv(const message::Execute &exec);
    bool handlePriv(const message::CancelExecute &cancel);
    bool handlePriv(const message::ExecutionProgress &prog);
    bool handlePriv(const message::Busy &busy);
    bool handlePriv(const message::Idle &idle);
    bool handlePriv(const message::SetParameter &setParam);
    bool handlePriv(const message::SetParameterChoices &setChoices, const MessagePayload &payload);
    bool handlePriv(const message::AddObject &addObj);
    bool handlePriv(const message::AddObjectCompleted &complete);
    bool handlePriv(const message::Barrier &barrier);
    bool handlePriv(const message::BarrierReached &barrierReached);
    bool handlePriv(const message::SendText &text, const MessagePayload &payload);
    bool handlePriv(const message::ItemInfo &info, const MessagePayload &payload);
    bool handlePriv(const message::RequestTunnel &tunnel);
    bool handlePriv(const message::Ping &ping);
    bool handlePriv(const message::DataTransferState &state);

    bool scanModules(const std::string &dir);

    //! check whether objects are available for a module and compute() should be called
    bool checkExecuteObject(int modId);

    const int m_rank;
    const int m_size;

    struct MessageWithPayload {
        MessageWithPayload(const message::Buffer &buf, const MessagePayload &pl = MessagePayload())
        : buf(buf), payload(pl)
        {}
        message::Buffer buf;
        MessagePayload payload;
    };
    std::mutex m_incomingMutex;
    std::deque<MessageWithPayload> m_incomingMessages;

    struct Module {
#ifdef MODULE_THREAD
        typedef std::shared_ptr<vistle::Module>(NewModuleFunc)(const std::string &name, int id, mpi::communicator comm);
        boost::function<NewModuleFunc> newModule;
        std::shared_ptr<vistle::Module> instance;
        std::thread thread;
#endif
        std::thread messageThread;
        std::shared_ptr<message::MessageQueue> sendQueue, recvQueue;
        int ranksStarted, ranksFinished;
        bool prepared, reduced;
        int busyCount;
        // handling of outgoing messages
        mutable bool blocked; // any message is blocking and cannot be sent right away
        mutable std::deque<message::Buffer> blockers; // queue of blocking messages
        mutable std::deque<MessageWithPayload> blockedMessages; // again, but with payload
        std::deque<MessageWithPayload>
            delayedMessages; // these messages have been held up for not disturbing their order
        // handling of incoming messages
        std::deque<MessageWithPayload> incomingMessages; // not yet processed, because module takes part in a barrier
        std::vector<int> objectCount; // no. of available object tuples on each rank

        Module(): ranksStarted(0), ranksFinished(0), prepared(false), reduced(true), busyCount(0), blocked(false) {}
        ~Module();

        void block(const message::Message &msg) const;
        void unblock(const message::Message &msg) const;
        bool send(const message::Message &msg, const MessagePayload &payload = MessagePayload()) const;
        bool update() const;
        void delay(const message::Message &msg, const MessagePayload &payload = MessagePayload());
        bool processDelayed(bool *haveExecute = nullptr);
        bool haveDelayed() const;
    };
    typedef std::unordered_map<int, Module> RunningMap;
    RunningMap runningMap;
    int numRunning() const;
    bool isReadyForExecute(int modId) const;
    int m_numExecuting = 0;

    // barrier related stuff
    bool checkBarrier(const message::uuid_t &uuid) const;
    void barrierReached(const message::uuid_t &uuid);
    bool m_barrierActive;
    message::uuid_t m_barrierUuid;
    int m_reachedBarriers;
    typedef std::set<int> ModuleSet;
    ModuleSet reachedSet;

    std::vector<int> m_numTransfering;
    long m_totalNumTransferring = 0;
    double m_lastStatusUpdateTime = 0.;

    CompressionSettings m_compressionSettings;
    bool m_compressionSettingsValid = false;
};

} // namespace vistle

#endif
