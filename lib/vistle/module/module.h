#ifndef MODULE_H
#define MODULE_H

/**
 \class Module

 \brief base class for Vistle modules

 Derive from Module, if you want to implement a module for processing data.
 You should reimplement the method @ref compute in your derived class.
 */

#if 0
#ifndef MPICH_IGNORE_CXX_SEEK
#define MPICH_IGNORE_CXX_SEEK
#endif
#include <mpi.h>
#else
#include <boost/mpi.hpp>
#endif
#include <boost/config.hpp>

#include <iostream>
#include <list>
#include <map>
#include <exception>
#include <deque>
#include <mutex>
#include <future>

#include <vistle/core/paramvector.h>
#include <vistle/core/object.h>
#include <vistle/core/parameter.h>
#include <vistle/core/port.h>
#include <vistle/core/grid.h>
#include <vistle/core/message.h>
#include <vistle/core/parametermanager.h>
#include <vistle/core/messagesender.h>
#include <vistle/core/messagepayload.h>

#include "objectcache.h"
#define RESULTCACHE_SKIP_DEFINITION
#include "resultcache.h"
#undef RESULTCACHE_SKIP_DEFINITION
#include "export.h"

#ifdef MODULE_THREAD
#ifdef MODULE_STATIC
#include "moduleregistry.h"
#else
#include <boost/dll/alias.hpp>
#endif
#endif

namespace mpi = ::boost::mpi;

namespace vistle {

class StateTracker;
struct HubData;
class Module;
class Renderer;

namespace message {
class Message;
class Execute;
class Buffer;
class AddParameter;
class SetParameter;
class RemoveParameter;
class MessageQueue;
} // namespace message

class V_MODULEEXPORT BlockTask {
    friend class Module;

public:
    explicit BlockTask(Module *module);
    virtual ~BlockTask();

    bool hasObject(const Port *p);
    Object::const_ptr takeObject(const Port *p);
    template<class Type>
    typename Type::const_ptr accept(const Port *port);
    template<class Type>
    typename Type::const_ptr accept(const std::string &port);
    template<class Type>
    typename Type::const_ptr expect(const Port *port);
    template<class Type>
    typename Type::const_ptr expect(const std::string &port);

    void addObject(Port *port, Object::ptr obj);
    void addObject(const std::string &port, Object::ptr obj);

protected:
    void addDependency(std::shared_ptr<BlockTask> dep);

    void addAllObjects();

    bool wait();
    bool waitDependencies();

    Module *m_module = nullptr;
    std::map<const Port *, Object::const_ptr> m_input;
    std::set<Port *> m_ports;
    std::map<std::string, Port *> m_portsByString;
    std::set<std::shared_ptr<BlockTask>> m_dependencies;
    std::map<Port *, std::deque<Object::ptr>> m_objects;

    std::mutex m_mutex;
    std::shared_future<bool> m_future;
};

class V_MODULEEXPORT Module: public ParameterManager, public MessageSender {
    friend class Reader;
    friend class Renderer;
    friend class Cache; // for passThroughObject
    friend class BlockTask;

public:
    static bool setup(const std::string &shmname, int moduleID, int rank);

    Module(const std::string &name, const int moduleID, mpi::communicator comm);
    virtual ~Module();
    const StateTracker &state() const;
    StateTracker &state();
    virtual void eventLoop(); // called from MODULE_MAIN
    void initDone(); // to be called from eventLoop after module ctor has run

    virtual bool dispatch(bool block = true, bool *messageReceived = nullptr, unsigned int minPrio = 0);

    Parameter *addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> parameter) override;
    bool removeParameter(Parameter *param) override;

    const std::string &name() const;
    const mpi::communicator &comm() const;
    const mpi::communicator &commShmGroup() const;
    int rank() const;
    int size() const;
    int shmLeader(int rank = -1) const; // return -1 or rank of leader of shm group, if rank is in current shm group
    int id() const;

    unsigned hardware_concurrency() const;

    ObjectCache::CacheMode setCacheMode(ObjectCache::CacheMode mode, bool update = true);
    ObjectCache::CacheMode cacheMode() const;

    Port *createInputPort(const std::string &name, const std::string &description = "", const int flags = 0);
    Port *createOutputPort(const std::string &name, const std::string &description = "", const int flags = 0);
    bool destroyPort(const std::string &portName);
    bool destroyPort(const Port *port);

    bool sendObject(const mpi::communicator &comm, vistle::Object::const_ptr object, int destRank) const;
    bool sendObject(vistle::Object::const_ptr object, int destRank) const;
    vistle::Object::const_ptr receiveObject(const mpi::communicator &comm, int destRank) const;
    vistle::Object::const_ptr receiveObject(int destRank) const;
    bool broadcastObject(const mpi::communicator &comm, vistle::Object::const_ptr &object, int root) const;
    bool broadcastObject(vistle::Object::const_ptr &object, int root) const;
    bool broadcastObjectViaShm(vistle::Object::const_ptr &object, const std::string &objName, int root) const;

    bool addObject(Port *port, vistle::Object::ptr object);
    bool addObject(const std::string &portName, vistle::Object::ptr object);

    ObjectList getObjects(const std::string &portName);
    bool hasObject(const Port *port) const;
    bool hasObject(const std::string &portName) const;
    vistle::Object::const_ptr takeFirstObject(Port *port);
    vistle::Object::const_ptr takeFirstObject(const std::string &portName);

    template<class Type>
    typename Type::const_ptr accept(Port *port);
    template<class Type>
    typename Type::const_ptr accept(const std::string &port);
    template<class Interface>
    const Interface *acceptInterface(Port *port);
    template<class Interface>
    const Interface *acceptInterface(const std::string &port);

    template<class Type>
    typename Type::const_ptr expect(Port *port);
    template<class Type>
    typename Type::const_ptr expect(const std::string &port);

    //! let module clear cache at appropriate times
    void addResultCache(ResultCacheBase &cache);

    //! request hub to forward incoming TCP/IP connections on forwardPort to be forwarded to localPort
    void requestPortMapping(unsigned short forwardPort, unsigned short localPort);
    //! remove port forwarding requested by requestPortMapping
    void removePortMapping(unsigned short forwardPort);

    void sendParameterMessage(const message::Message &message, const buffer *payload) const override;
    bool sendMessage(const message::Message &message, const buffer *payload = nullptr) const override;
    bool sendMessage(const message::Message &message, const MessagePayload &payload) const override;
    template<class Payload>
    bool sendMessageWithPayload(message::Message &message, Payload &payload) const;

    //! type should be a message::SendText::TextType
    void sendText(int type, const std::string &msg) const;

    /// send info message to UI - printf style
    void sendInfo(const char *fmt, ...) const
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        ;

    /// send warning message to UI - printf style
    void sendWarning(const char *fmt, ...) const
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        ;

    /// send error message to UI - printf style
    void sendError(const char *fmt, ...) const
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        ;

    /// send response message to UI - printf style
    void sendError(const message::Message &msg, const char *fmt, ...) const
#ifdef __GNUC__
        __attribute__((format(printf, 3, 4)))
#endif
        ;

    /// send info message to UI - string style
    void sendInfo(const std::string &text) const;
    /// send warning message to UI - string style
    void sendWarning(const std::string &text) const;
    /// send error message to UI - string style
    void sendError(const std::string &text) const;
    /// send response message to UI - string style
    void sendError(const message::Message &msg, const std::string &text) const;

    int schedulingPolicy() const;
    void setSchedulingPolicy(int schedulingPolicy /*< really message::SchedulingPolicy::Schedule */);

    int reducePolicy() const;
    void setReducePolicy(int reduceRequirement /*< really message::ReducePolicy::Reduce */);

    void virtual prepareQuit();

    const HubData &getHub() const;

    bool isConnected(const Port &port) const;
    bool isConnected(const std::string &portname) const;
    std::string getModuleName(int id) const;
    int mirrorId() const;
    std::set<int> getMirrors() const;
    //request execution of this module
    void execute() const;

    void updateMeta(vistle::Object::ptr object) const;

protected:
    bool passThroughObject(Port *port, vistle::Object::const_ptr object);
    bool passThroughObject(const std::string &portName, vistle::Object::const_ptr object);

    void setObjectReceivePolicy(int pol);
    int objectReceivePolicy() const;
    void startIteration(); //< increase iteration counter

    const std::string m_name;
    int m_rank;
    int m_size;
    const int m_id;

    int m_executionCount, m_iteration;
    std::set<Port *> m_withOutput;

    void setDefaultCacheMode(ObjectCache::CacheMode mode);

    message::MessageQueue *sendMessageQueue;
    message::MessageQueue *receiveMessageQueue;
    std::deque<message::Buffer> messageBacklog;
    virtual bool handleMessage(const message::Message *message, const vistle::MessagePayload &payload);
    virtual bool handleExecute(const message::Execute *exec);
    bool cancelRequested(bool collective = false);
    bool wasCancelRequested() const;
    virtual void cancelExecuteMessageReceived(const message::Message *msg);
    virtual bool addInputObject(int sender, const std::string &senderPort, const std::string &portName,
                                Object::const_ptr object);
    virtual bool
    objectAdded(int sender, const std::string &senderPort,
                const Port *port); //< notification when data object has been added - called on each rank individually

    bool syncMessageProcessing() const;
    void setSyncMessageProcessing(bool sync);

    virtual void connectionAdded(const Port *from, const Port *to);
    virtual void connectionRemoved(const Port *from, const Port *to);


    bool changeParameter(const Parameter *p) override;

    int openmpThreads() const;
    void setOpenmpThreads(int, bool updateParam = true);

    void enableBenchmark(bool benchmark, bool updateParam = true);

    virtual bool prepare(); //< prepare execution - called on each rank individually
    virtual bool reduce(int timestep); //< do reduction for timestep (-1: global) - called on all ranks
    virtual bool cancelExecute(); //< if execution has been canceled early before all objects have been processed
    int numTimesteps() const;

    void setStatus(const std::string &text, message::UpdateStatus::Importance prio = message::UpdateStatus::Low);
    void clearStatus();

    bool getNextMessage(message::Buffer &buf, bool block = true, unsigned int minPrio = 0);

    bool reduceWrapper(const message::Execute *exec, bool reordered = false);
    bool prepareWrapper(const message::Execute *exec);

    void clearResultCaches();
    void enableResultCaches(bool on);

private:
    std::shared_ptr<StateTracker> m_stateTracker;
    int m_receivePolicy;
    int m_schedulingPolicy;
    int m_reducePolicy;

    bool havePort(const std::string &name); //< check whether a port or parameter already exists
    Port *findInputPort(const std::string &name);
    const Port *findInputPort(const std::string &name) const;
    Port *findOutputPort(const std::string &name);
    const Port *findOutputPort(const std::string &name) const;

    bool needsSync(const message::Message &m) const;

    //! notify that a module has added a parameter
    virtual bool parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg,
                                const std::string &moduleName);
    //! notify that a module modified a parameter value
    virtual bool parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg);
    //! notify that a module removed a parameter
    virtual bool parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg);

    virtual bool compute(); //< do processing - called on each rank individually
    virtual bool compute(std::shared_ptr<BlockTask> task) const;

    std::map<std::string, Port> outputPorts;
    std::map<std::string, Port> inputPorts;

    ObjectCache m_cache;
    ObjectCache::CacheMode m_defaultCacheMode;
    bool m_prioritizeVisible;
    void updateCacheMode();
    bool m_syncMessageProcessing;

    IntParameter *m_useResultCache = nullptr;
    std::vector<ResultCacheBase *> m_resultCaches;

    void updateOutputMode();
    std::streambuf *m_origStreambuf = nullptr, *m_streambuf = nullptr;

    int m_traceMessages;
    bool m_benchmark;
    double m_benchmarkStart;
    double m_avgComputeTime;
    mpi::communicator m_comm, m_commShmGroup, m_commShmLeaders;
    std::vector<int> m_shmLeaders; // leader rank in m_comm of m_commShmGroup for every rank in m_comm
    std::vector<int> m_shmLeadersSubrank; // leader rank in m_commShmLeaders of m_commShmGroup for every rank in m_comm

    int m_numTimesteps;
    bool m_cancelRequested = false, m_cancelExecuteCalled = false, m_executeAfterCancelFound = false;
    bool m_upstreamIsExecuting = false, m_prepared = false, m_computed = false, m_reduced = false;
    bool m_readyForQuit = false;

    std::unique_ptr<vistle::message::Buffer> m_delayedBarrierResponse;

    //maximum number of parallel threads per rank
    IntParameter *m_concurrency = nullptr;
    void waitAllTasks();
    std::shared_ptr<BlockTask> m_lastTask;
    std::deque<std::shared_ptr<BlockTask>> m_tasks;

    unsigned m_hardware_concurrency = 1;
};

V_MODULEEXPORT int getTimestep(Object::const_ptr obj);
V_MODULEEXPORT double getRealTime(Object::const_ptr obj);

template<>
V_MODULEEXPORT Object::const_ptr Module::expect<Object>(Port *port);

} // namespace vistle

#ifdef MODULE_THREAD
#ifdef MODULE_STATIC
#define MODULE_MAIN_THREAD(X, THREAD_MODE) \
    static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId, \
                                                             mpi::communicator comm) \
    { \
        vistle::Module::setup("dummy shm", moduleId, comm.rank()); \
        return std::shared_ptr<X>(new X(name, moduleId, comm)); \
    } \
    static vistle::ModuleRegistry::RegisterClass registerModule##X(VISTLE_MODULE_NAME, newModuleInstance);
#else
#define MODULE_MAIN_THREAD(X, THREAD_MODE) \
    static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId, \
                                                             mpi::communicator comm) \
    { \
        vistle::Module::setup("dummy shm", moduleId, comm.rank()); \
        return std::shared_ptr<X>(new X(name, moduleId, comm)); \
    } \
    BOOST_DLL_ALIAS(newModuleInstance, newModule)
#endif

#define MODULE_DEBUG(X)
#else
// MPI_THREAD_FUNNELED is sufficient, but apparently not provided by the CentOS build of MVAPICH2
#define MODULE_MAIN_THREAD(X, THREAD_MODE) \
    int main(int argc, char **argv) \
    { \
        if (argc != 4) { \
            std::cerr << "module requires exactly 4 parameters" << std::endl; \
            exit(1); \
        } \
        int rank = -1, size = -1; \
        try { \
            std::string shmname = argv[1]; \
            const std::string name = argv[2]; \
            int moduleID = atoi(argv[3]); \
            mpi::environment mpi_environment(argc, argv, THREAD_MODE, true); \
            vistle::registerTypes(); \
            mpi::communicator comm_world; \
            rank = comm_world.rank(); \
            size = comm_world.size(); \
            vistle::Module::setup(shmname, moduleID, rank); \
            { \
                X module(name, moduleID, comm_world); \
                module.eventLoop(); \
            } \
            comm_world.barrier(); \
        } catch (vistle::exception & e) { \
            std::cerr << "[" << rank << "/" << size << "]: fatal exception: " << e.what() << std::endl; \
            std::cerr << "  info: " << e.info() << std::endl; \
            std::cerr << e.where() << std::endl; \
            exit(1); \
        } catch (std::exception & e) { \
            std::cerr << "[" << rank << "/" << size << "]: fatal exception: " << e.what() << std::endl; \
            exit(1); \
        } \
        return 0; \
    }

#ifdef NDEBUG
#define MODULE_DEBUG(X)
#else
#define MODULE_DEBUG(X) \
    std::cerr << #X << ": PID " << get_process_handle() << std::endl; \
    std::cerr << "   attach debugger within 10 s" << std::endl; \
    sleep(10); \
    std::cerr << "   continuing..." << std::endl;
#endif
#endif

#define MODULE_MAIN(X) MODULE_MAIN_THREAD(X, boost::mpi::threading::funneled)

#include "module_impl.h"
#endif
