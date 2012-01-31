#ifndef MODULE_H
#define MODULE_H

#define MPICH_IGNORE_CXX_SEEK
#include <mpi.h>

#include <iostream>
#include <list>
#include <map>

#include "vector.h"

namespace vistle {

class Parameter;

namespace message {
class Message;
class MessageQueue;
}

class Object;

class Module {

 public:
   Module(const std::string &name,
          const int rank, const int size, const int moduleID);
   virtual ~Module();

   virtual bool dispatch();

 protected:

   bool createInputPort(const std::string & name);
   bool createOutputPort(const std::string & name);

   bool addFileParameter(const std::string & name, const std::string & value);
   void setFileParameter(const std::string & name, const std::string & value);
   std::string getFileParameter(const std::string & name) const;

   bool addFloatParameter(const std::string & name, const float value);
   void setFloatParameter(const std::string & name, const float value);
   float getFloatParameter(const std::string & name) const;

   bool addIntParameter(const std::string & name, const int value);
   void setIntParameter(const std::string & name, const int value);
   int getIntParameter(const std::string & name) const;

   bool addVectorParameter(const std::string & name, const Vector & value);
   void setVectorParameter(const std::string & name, const Vector & value);
   Vector getVectorParameter(const std::string & name) const;

   bool addObject(const std::string &portName, const shm_handle_t & handle);
   bool addObject(const std::string & portName, const void * object);
   message::MessageQueue *sendMessageQueue;

   std::list<vistle::Object *> getObjects(const std::string &portName);
   void removeObject(const std::string &portName, vistle::Object * object);

   const std::string name;
   const int rank;
   const int size;
   const int moduleID;

 protected:
   message::MessageQueue *receiveMessageQueue;
   bool handleMessage(const message::Message *message);

 private:
   bool addInputObject(const std::string & portName,
                       const shm_handle_t & handle);

   virtual bool addInputObject(const std::string & portName,
                               const Object * object);

   virtual bool compute() = 0;

   std::map<std::string, std::list<shm_handle_t> *> outputPorts;
   std::map<std::string, std::list<shm_handle_t> *> inputPorts;

   std::map<std::string, Parameter *> parameters;
};

} // namespace vistle

#define MODULE_MAIN(X) int main(int argc, char **argv) {        \
      int rank, size, moduleID;                                 \
      if (argc != 2) {                                          \
         std::cerr << "module missing parameters" << std::endl; \
         exit(1);                                               \
      }                                                         \
      MPI_Init(&argc, &argv);                                   \
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);                     \
      MPI_Comm_size(MPI_COMM_WORLD, &size);                     \
      moduleID = atoi(argv[1]);                                 \
      X module(rank, size, moduleID);                           \
      while (!module.dispatch());                               \
      MPI_Barrier(MPI_COMM_WORLD);                              \
      return 0;                                                 \
   }

#endif
