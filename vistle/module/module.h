#ifndef MODULE_H
#define MODULE_H

#if 0
#ifndef MPICH_IGNORE_CXX_SEEK
#define MPICH_IGNORE_CXX_SEEK
#endif
#include <mpi.h>
#else
#include <boost/mpi.hpp>
#include <boost/serialization/vector.hpp>
#endif
#include <boost/config.hpp>

#include <iostream>
#include <list>
#include <map>
#include <exception>

#include <core/paramvector.h>
#include <core/object.h>
#include <core/parameter.h>
#include <core/port.h>
#include <core/grid.h>
#include <core/message.h>
#include <core/parametermanager.h>
#include <core/messagesender.h>

#include "objectcache.h"
#include "export.h"

#ifdef MODULE_THREAD
#ifdef MODULE_STATIC
#include "moduleregistry.h"
#else
#include <boost/dll/alias.hpp>
#endif
#endif

namespace mpi = boost::mpi;

namespace vistle {

class StateTracker;
struct HubData;
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

class V_MODULEEXPORT Module: public ParameterManager, public MessageSender {
   friend class Renderer;

 public:
   static bool setup(const std::string &shmname, int moduleID, int rank);

   Module(const std::string &description,
          const std::string &name, const int moduleID, mpi::communicator comm);
   virtual ~Module();
   virtual void eventLoop(); // called from MODULE_MAIN
   void initDone(); // to be called from eventLoop after module ctor has run

   virtual bool dispatch(bool *messageReived=nullptr);

   Parameter *addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> parameter) override;
   bool removeParameter(Parameter *param) override;

   const std::string &name() const;
   const boost::mpi::communicator &comm() const;
   int rank() const;
   int size() const;
   int id() const;

   ObjectCache::CacheMode setCacheMode(ObjectCache::CacheMode mode, bool update=true);
   ObjectCache::CacheMode cacheMode(ObjectCache::CacheMode mode) const;

   Port *createInputPort(const std::string &name, const std::string &description="", const int flags=0);
   Port *createOutputPort(const std::string &name, const std::string &description="", const int flags=0);
   bool destroyPort(const std::string &portName);
   bool destroyPort(Port *port);

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

   void sendParameterMessage(const message::Message &message) const override;
   bool sendMessage(const message::Message &message) const override;

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
   virtual bool handleMessage(const message::Message *message);
   bool handleExecute(const message::Execute *exec);
   bool cancelRequested(bool collective=false);

   virtual bool addInputObject(int sender, const std::string &senderPort, const std::string & portName,
                               Object::const_ptr object);
   virtual bool objectAdded(int sender, const std::string &senderPort, const Port *port); //< notification when data object has been added - called on each rank individually

   bool syncMessageProcessing() const;
   void setSyncMessageProcessing(bool sync);

   bool isConnected(const Port *port) const;
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

 private:
   bool reduceWrapper(const message::Execute *exec, bool reordered=false);
   bool prepareWrapper(const message::Execute *exec);

   std::shared_ptr<StateTracker> m_stateTracker;
   int m_receivePolicy;
   int m_schedulingPolicy;
   int m_reducePolicy;
   int m_executionDepth; //< number of input ports that have sent ExecutionProgress::Start

   bool havePort(const std::string &name); //< check whether a port or parameter already exists
   Port *findInputPort(const std::string &name) const;
   Port *findOutputPort(const std::string &name) const;

   bool needsSync(const message::Message &m) const;

   //! notify that a module has added a parameter
   virtual bool parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName);
   //! notify that a module modified a parameter value
   virtual bool parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg);
   //! notify that a module removed a parameter
   virtual bool parameterRemoved(const int senderId, const std::string &name, const message::RemoveParameter &msg);

   virtual bool compute(); //< do processing - called on each rank individually

   std::map<std::string, Port*> outputPorts;
   std::map<std::string, Port*> inputPorts;

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
   boost::mpi::communicator m_comm;

   int m_numTimesteps;
   bool m_cancelRequested, m_cancelExecuteCalled;
   bool m_prepared, m_computed, m_reduced;
   bool m_readyForQuit;
};

template<>
V_MODULEEXPORT Object::const_ptr Module::expect<Object>(Port *port);

} // namespace vistle

// MPI_THREAD_SINGLE seems to be ok for OpenMP with MPICH
#ifdef MPICH_VERSION
#define V_HAVE_MPICH 0
#else
#define V_HAVE_MPICH 0
#endif

#ifdef MODULE_THREAD
#ifdef MODULE_STATIC
#define MODULE_MAIN(X) \
    static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId, boost::mpi::communicator comm) { \
       vistle::Module::setup("dummy shm", moduleId, comm.rank()); \
       return std::shared_ptr<X>(new X(name, moduleId, comm)); \
    } \
    static vistle::ModuleRegistry::RegisterClass registerModule(VISTLE_MODULE_NAME, newModuleInstance);
#else
#define MODULE_MAIN(X) \
    static std::shared_ptr<vistle::Module> newModuleInstance(const std::string &name, int moduleId, boost::mpi::communicator comm) { \
       vistle::Module::setup("dummy shm", moduleId, comm.rank()); \
       return std::shared_ptr<X>(new X(name, moduleId, comm)); \
    } \
    BOOST_DLL_ALIAS(newModuleInstance, newModule);
#endif

#define MODULE_DEBUG(X)
#else
// MPI_THREAD_FUNNELED is sufficient, but apparantly not provided by the CentOS build of MVAPICH2
#define MODULE_MAIN(X) \
   int main(int argc, char **argv) { \
      int provided = MPI_THREAD_SINGLE; \
      MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); \
      if (provided == MPI_THREAD_SINGLE && !V_HAVE_MPICH) { \
         std::cerr << "no thread support in MPI" << std::endl; \
         exit(1); \
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
            X module(name, moduleID, boost::mpi::communicator()); \
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
   std::cerr << #X << ": PID " << getpid() << std::endl; \
   std::cerr << "   attach debugger within 10 s" << std::endl; \
   sleep(10); \
   std::cerr << "   continuing..." << std::endl;
#endif
#endif

#ifdef VISTLE_IMPL
#include "module_impl.h"
#endif
#endif
