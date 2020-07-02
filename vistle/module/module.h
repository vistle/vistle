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

#include <core/paramvector.h>
#include <core/object.h>
#include <core/parameter.h>
#include <core/port.h>
#include <core/grid.h>
#include <core/message.h>
#include <core/parametermanager.h>
#include <core/messagesender.h>
#include <core/messagepayload.h>

#include "objectcache.h"
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
}

class V_MODULEEXPORT PortTask {

    friend class Module;

public:
    explicit PortTask(Module *module);
    virtual ~PortTask();

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

    void addDependency(std::shared_ptr<PortTask> dep);
    void addObject(Port *port, Object::ptr obj);
    void addObject(const std::string &port, Object::ptr obj);
    void passThroughObject(Port *port, Object::const_ptr obj);
    void passThroughObject(const std::string &port, Object::const_ptr obj);

    void addAllObjects();

    bool isDone();
    bool dependenciesDone();

    bool wait();
    bool waitDependencies();

protected:
    Module *m_module = nullptr;
    std::map<const Port *, Object::const_ptr> m_input;
    std::set<Port *> m_ports;
    std::map<std::string, Port *> m_portsByString;
    std::set<std::shared_ptr<PortTask>> m_dependencies;
    std::map<Port *, std::deque<Object::ptr>> m_objects;
    std::map<Port *, std::deque<bool>> m_passThrough;

    std::mutex m_mutex;
    std::shared_future<bool> m_future;
};

class V_MODULEEXPORT Module: public ParameterManager, public MessageSender {
   friend class Reader;
   friend class Renderer;
   friend class PortTask;

 public:
   static bool setup(const std::string &shmname, int moduleID, int rank);

   Module(const std::string &description,
          const std::string &name, const int moduleID, mpi::communicator comm);
   virtual ~Module();
   virtual void eventLoop(); // called from MODULE_MAIN
   void initDone(); // to be called from eventLoop after module ctor has run

   virtual bool dispatch(bool block = true, bool * messageReceived =nullptr);

   Parameter *addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> parameter) override;
   bool removeParameter(Parameter *param) override;

   const std::string &name() const;
   const mpi::communicator &comm() const;
   int rank() const;
   int size() const;
   int id() const;

   unsigned hardware_concurrency() const;

   ObjectCache::CacheMode setCacheMode(ObjectCache::CacheMode mode, bool update=true);
   ObjectCache::CacheMode cacheMode() const;

   Port *createInputPort(const std::string &name, const std::string &description="", const int flags=0);
   Port *createOutputPort(const std::string &name, const std::string &description="", const int flags=0);
   bool destroyPort(const std::string &portName);
   bool destroyPort(const Port *port);

   bool sendObject(const mpi::communicator &comm, vistle::Object::const_ptr object, int destRank) const;
   bool sendObject(vistle::Object::const_ptr object, int destRank) const;
   vistle::Object::const_ptr receiveObject(const mpi::communicator &comm, int destRank) const;
   vistle::Object::const_ptr receiveObject(int destRank) const;
   bool broadcastObject(const mpi::communicator &comm, vistle::Object::const_ptr &object, int root) const;
   bool broadcastObject(vistle::Object::const_ptr &object, int root) const;

   bool addObject(Port *port, vistle::Object::ptr object);
   bool addObject(const std::string &portName, vistle::Object::ptr object);
   bool passThroughObject(Port *port, vistle::Object::const_ptr object);
   bool passThroughObject(const std::string &portName, vistle::Object::const_ptr object);

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

   //! request hub to forward incoming connections on forwardPort to be forwarded to localPort
   void requestPortMapping(unsigned short forwardPort, unsigned short localPort);
   //! remove port forwarding requested by requestPortMapping
   void removePortMapping(unsigned short forwardPort);

   void sendParameterMessage(const message::Message &message, const buffer *payload) const override;
   bool sendMessage(const message::Message &message, const buffer *payload=nullptr) const override;
   template<class Payload>
   bool sendMessage(message::Message &message, Payload &payload) const;

   //! type should be a message::SendText::TextType
   void sendText(int type, const std::string &msg) const;

   /// send info message to UI - printf style
   void sendInfo(const char *fmt, ...) const
#ifdef __GNUC__
   __attribute__ ((format (printf, 2, 3)))
#endif
   ;

   /// send warning message to UI - printf style
   void sendWarning(const char *fmt, ...) const
#ifdef __GNUC__
   __attribute__ ((format (printf, 2, 3)))
#endif
   ;

   /// send error message to UI - printf style
   void sendError(const char *fmt, ...) const
#ifdef __GNUC__
   __attribute__ ((format (printf, 2, 3)))
#endif
   ;

   /// send response message to UI - printf style
   void sendError(const message::Message &msg, const char *fmt, ...) const
#ifdef __GNUC__
   __attribute__ ((format (printf, 3, 4)))
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

protected:

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
   void updateMeta(vistle::Object::ptr object) const;

   message::MessageQueue *sendMessageQueue;
   message::MessageQueue *receiveMessageQueue;
   std::deque<message::Buffer> messageBacklog;
   virtual bool handleMessage(const message::Message *message, const vistle::MessagePayload &payload);
   virtual bool handleExecute(const message::Execute *exec);
   bool cancelRequested(bool collective=false);
   bool wasCancelRequested() const;
   virtual void cancelExecuteMessageReceived(const message::Message* msg);
   virtual bool addInputObject(int sender, const std::string &senderPort, const std::string & portName,
                               Object::const_ptr object);
   virtual bool objectAdded(int sender, const std::string &senderPort, const Port *port); //< notification when data object has been added - called on each rank individually

   bool syncMessageProcessing() const;
   void setSyncMessageProcessing(bool sync);

   bool isConnected(const Port &port) const;
   bool isConnected(const std::string &portname) const;
   virtual void connectionAdded(const Port *from, const Port *to);
   virtual void connectionRemoved(const Port *from, const Port *to);

   std::string getModuleName(int id) const;

   bool changeParameter(const Parameter *p) override;

   int openmpThreads() const;
   void setOpenmpThreads(int, bool updateParam=true);

   void enableBenchmark(bool benchmark, bool updateParam=true);

   virtual bool prepare(); //< prepare execution - called on each rank individually
   virtual bool reduce(int timestep); //< do reduction for timestep (-1: global) - called on all ranks
   virtual bool cancelExecute(); //< if execution has been canceled early before all objects have been processed
   int numTimesteps() const;

   void setStatus(const std::string &text, message::UpdateStatus::Importance prio=message::UpdateStatus::Low);
   void clearStatus();

   bool getNextMessage(message::Buffer &buf, bool block=true);

   bool reduceWrapper(const message::Execute* exec, bool reordered = false);
   bool prepareWrapper(const message::Execute* exec);

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
   virtual bool parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName);
   //! notify that a module modified a parameter value
   virtual bool parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg);
   //! notify that a module removed a parameter
   virtual bool parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg);

   virtual bool compute(); //< do processing - called on each rank individually
   virtual bool compute(std::shared_ptr<PortTask> task) const;

   std::map<std::string, Port> outputPorts;
   std::map<std::string, Port> inputPorts;

   ObjectCache m_cache;
   ObjectCache::CacheMode m_defaultCacheMode;
   bool m_prioritizeVisible;
   void updateCacheMode();
   bool m_syncMessageProcessing;

   void updateOutputMode();
   std::streambuf *m_origStreambuf, *m_streambuf;

   int m_traceMessages;
   bool m_benchmark;
   double m_benchmarkStart;
   double m_avgComputeTime;
   mpi::communicator m_comm;

   int m_numTimesteps;
   bool m_cancelRequested=false, m_cancelExecuteCalled=false, m_executeAfterCancelFound=false;
   bool m_prepared, m_computed, m_reduced;
   bool m_readyForQuit;

   IntParameter *m_concurrency = nullptr;
   void waitAllTasks();
   std::shared_ptr<PortTask> m_lastTask;
   std::deque<std::shared_ptr<PortTask>> m_tasks;

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
    static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId, mpi::communicator comm) { \
       vistle::Module::setup("dummy shm", moduleId, comm.rank()); \
       return std::shared_ptr<X>(new X(name, moduleId, comm)); \
    } \
    static vistle::ModuleRegistry::RegisterClass registerModule##X(VISTLE_MODULE_NAME, newModuleInstance);
#else
#define MODULE_MAIN_THREAD(X, THREAD_MODE) \
    static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId, mpi::communicator comm) { \
       vistle::Module::setup("dummy shm", moduleId, comm.rank()); \
       return std::shared_ptr<X>(new X(name, moduleId, comm)); \
    } \
    BOOST_DLL_ALIAS(newModuleInstance, newModule)
#endif

#define MODULE_DEBUG(X)
#else
// MPI_THREAD_FUNNELED is sufficient, but apparantly not provided by the CentOS build of MVAPICH2
#define MODULE_MAIN_THREAD(X, THREAD_MODE) \
   int main(int argc, char **argv) { \
      int provided = MPI_THREAD_SINGLE; \
      MPI_Init_thread(&argc, &argv, THREAD_MODE, &provided); \
      if (provided == MPI_THREAD_SINGLE && THREAD_MODE != MPI_THREAD_SINGLE) { \
         std::cerr << "no thread support in MPI, continuing anyway" << std::endl; \
      } \
      vistle::registerTypes(); \
      int rank=-1, size=-1; \
      std::string shmname; \
      try { \
         if (argc != 4) { \
            std::cerr << "module requires exactly 4 parameters" << std::endl; \
            MPI_Finalize(); \
            exit(1); \
         } \
         MPI_Comm_rank(MPI_COMM_WORLD, &rank); \
         MPI_Comm_size(MPI_COMM_WORLD, &size); \
         shmname = argv[1]; \
         const std::string name = argv[2]; \
         int moduleID = atoi(argv[3]); \
         vistle::Module::setup(shmname, moduleID, rank); \
         { \
            X module(name, moduleID, mpi::communicator()); \
            module.eventLoop(); \
         } \
         MPI_Barrier(MPI_COMM_WORLD); \
      } catch(vistle::exception &e) { \
         std::cerr << "[" << rank << "/" << size << "]: fatal exception: " << e.what() << std::endl; \
         std::cerr << "  info: " << e.info() << std::endl; \
         std::cerr << e.where() << std::endl; \
         MPI_Finalize(); \
         exit(1); \
      } catch(std::exception &e) { \
         std::cerr << "[" << rank << "/" << size << "]: fatal exception: " << e.what() << std::endl; \
         MPI_Finalize(); \
         exit(1); \
      } \
      MPI_Finalize(); \
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

#define MODULE_MAIN(X) MODULE_MAIN_THREAD(X, MPI_THREAD_FUNNELED)

#ifdef VISTLE_IMPL
#include "module_impl.h"
#endif
#endif
