#ifndef MODULE_H
#define MODULE_H

#define MPICH_IGNORE_CXX_SEEK
#include <mpi.h>

#include <iostream>
#include <list>
#include <map>
#include <exception>

#include "vector.h"
#include "object.h"
#include "export.h"

namespace vistle {

class Parameter;

namespace message {
struct Message;
class MessageQueue;
}

class VCEXPORT Module {

 public:
   Module(const std::string &name, const std::string &shmname,
          const unsigned int rank, const unsigned int size, const int moduleID);
   virtual ~Module();

   virtual bool dispatch();

   const std::string &name() const;
   unsigned int rank() const;
   unsigned int size() const;
   int id() const;

   bool createInputPort(const std::string & name);
   bool createOutputPort(const std::string & name);

   bool addParameterGeneric(const std::string &name, Parameter *parameter);
   bool updateParameter(const std::string &name, const Parameter *parameter);

   template<class T>
   bool addParameter(const std::string &name, const T &value);
   template<class T>
   bool setParameter(const std::string &name, const T &value);
   template<class T>
   bool getParameter(const std::string &name, T &value) const;

   bool addStringParameter(const std::string & name, const std::string & value);
   bool setStringParameter(const std::string & name, const std::string & value);
   std::string getStringParameter(const std::string & name) const;

   bool addFloatParameter(const std::string & name, const vistle::Scalar value);
   bool setFloatParameter(const std::string & name, const vistle::Scalar value);
   vistle::Scalar getFloatParameter(const std::string & name) const;

   bool addIntParameter(const std::string & name, const int value);
   bool setIntParameter(const std::string & name, const int value);
   int getIntParameter(const std::string & name) const;

   bool addVectorParameter(const std::string & name, const Vector & value);
   bool setVectorParameter(const std::string & name, const Vector & value);
   Vector getVectorParameter(const std::string & name) const;

   bool addObject(const std::string & portName, vistle::Object::const_ptr object);

   typedef std::list<vistle::Object::const_ptr> ObjectList;
   ObjectList getObjects(const std::string &portName);
   void removeObject(const std::string &portName, vistle::Object::const_ptr object);
   bool hasObject(const std::string &portName) const;
   vistle::Object::const_ptr takeFirstObject(const std::string &portName);

 protected:

   const std::string m_name;
   const unsigned int m_rank;
   const unsigned int m_size;
   const int m_id;

   message::MessageQueue *sendMessageQueue;
   message::MessageQueue *receiveMessageQueue;
   bool handleMessage(const message::Message *message);
   void sendMessage(const message::Message &message);

 private:

   virtual bool addInputObject(const std::string & portName,
                               Object::const_ptr object);

   virtual bool compute() = 0;

   std::map<std::string, ObjectList> outputPorts;
   std::map<std::string, ObjectList> inputPorts;

   std::map<std::string, Parameter *> parameters;
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
