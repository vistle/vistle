#ifndef MODULE_H
#define MODULE_H

#ifndef MPICH_IGNORE_CXX_SEEK
#define MPICH_IGNORE_CXX_SEEK
#endif
#include <mpi.h>

#include <iostream>
#include <list>
#include <map>
#include <exception>

#include "paramvector.h"
#include "object.h"
#include "export.h"
#include "objectcache.h"
#include "parameter.h"
#include "port.h"

namespace vistle {

namespace message {
struct Message;
struct AddParameter;
struct SetParameter;
class MessageQueue;
}

class V_COREEXPORT Module {

 public:
   Module(const std::string &name, const std::string &shmname,
          const unsigned int rank, const unsigned int size, const int moduleID);
   virtual ~Module();

   virtual bool dispatch();

   const std::string &name() const;
   unsigned int rank() const;
   unsigned int size() const;
   int id() const;

   void setCacheMode(ObjectCache::CacheMode mode);
   ObjectCache::CacheMode cacheMode(ObjectCache::CacheMode mode) const;

   Port *createInputPort(const std::string &name, const std::string &description="", const int flags=0);
   Port *createOutputPort(const std::string &name, const std::string &description="", const int flags=0);

   bool addParameterGeneric(const std::string &name, Parameter *parameter, Parameter::Presentation presentation);
   bool updateParameter(const std::string &name, const Parameter *parameter, bool reply);

   template<class T>
   Parameter *addParameter(const std::string &name, const std::string &description, const T &value, Parameter::Presentation presentation=Parameter::Generic);
   template<class T>
   bool setParameter(const std::string &name, const T &value, bool reply);
   template<class T>
   bool getParameter(const std::string &name, T &value) const;

   StringParameter *addStringParameter(const std::string & name, const std::string &description, const std::string & value, Parameter::Presentation p=Parameter::Generic);
   bool setStringParameter(const std::string & name, const std::string & value, bool reply=false);
   std::string getStringParameter(const std::string & name) const;

   FloatParameter *addFloatParameter(const std::string & name, const std::string &description, const vistle::Scalar value);
   bool setFloatParameter(const std::string & name, const vistle::Scalar value, bool reply=false);
   vistle::Scalar getFloatParameter(const std::string & name) const;

   IntParameter *addIntParameter(const std::string & name, const std::string &description, const int value, Parameter::Presentation p=Parameter::Generic);
   bool setIntParameter(const std::string & name, const int value, bool reply=false);
   int getIntParameter(const std::string & name) const;

   VectorParameter *addVectorParameter(const std::string & name, const std::string &description, const ParamVector & value);
   bool setVectorParameter(const std::string & name, const ParamVector & value, bool reply=false);
   ParamVector getVectorParameter(const std::string & name) const;

   bool addObject(const std::string & portName, vistle::Object::ptr object);
   bool passThroughObject(const std::string & portName, vistle::Object::const_ptr object);

   ObjectList getObjects(const std::string &portName);
   void removeObject(const std::string &portName, vistle::Object::const_ptr object);
   bool hasObject(const std::string &portName) const;
   vistle::Object::const_ptr takeFirstObject(const std::string &portName);

   void sendMessage(const message::Message &message) const;
 protected:

   const std::string m_name;
   const unsigned int m_rank;
   const unsigned int m_size;
   const int m_id;
   bool m_mpiFinalize;

   int m_executionCount;

   void setDefaultCacheMode(ObjectCache::CacheMode mode);
   void updateMeta(vistle::Object::ptr object) const;

   message::MessageQueue *sendMessageQueue;
   message::MessageQueue *receiveMessageQueue;
   bool handleMessage(const message::Message *message);

 private:
   Parameter *findParameter(const std::string &name) const;
   Port *findInputPort(const std::string &name) const;
   Port *findOutputPort(const std::string &name) const;

   virtual bool addInputObject(const std::string & portName,
                               Object::const_ptr object);
   virtual bool parameterAdded(const int senderId, const std::string &name, const message::AddParameter &msg, const std::string &moduleName);
   virtual bool parameterChanged(const int senderId, const std::string &name, const message::SetParameter &msg);

   virtual bool compute() = 0;

   std::map<std::string, Port*> outputPorts;
   std::map<std::string, Port*> inputPorts;

   std::map<std::string, Parameter *> parameters;
   ObjectCache m_cache;
   ObjectCache::CacheMode m_defaultCacheMode;
};

} // namespace vistle

#define MODULE_MAIN(X) \
   int main(int argc, char **argv) { \
      int rank, size, moduleID; \
      try { \
         if (argc != 3) { \
            std::cerr << "module requires exactly 2 parameters" << std::endl; \
            exit(1); \
         } \
         MPI_Init(&argc, &argv); \
         MPI_Comm_rank(MPI_COMM_WORLD, &rank); \
         MPI_Comm_size(MPI_COMM_WORLD, &size); \
         const std::string &shmname = argv[1]; \
         moduleID = atoi(argv[2]); \
         X module(shmname, rank, size, moduleID); \
         while (module.dispatch()) \
            ; \
         MPI_Barrier(MPI_COMM_WORLD); \
      } catch(std::exception &e) { \
         std::cerr << "fatal exception: " << e.what() << std::endl; \
         exit(1); \
      } \
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
