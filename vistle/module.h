#ifndef MODULE_H
#define MODULE_H

#include <iostream>
#include <mpi.h>

#include <list>
#include <map>

namespace vistle {

namespace message {
class Message;
}

class MessageQueue;

class Module {

 public:
   Module(const std::string &name,
          const int rank, const int size, const int moduleID);
   virtual ~Module();

   bool dispatch();

 protected:

   bool createInputPort(const std::string & name);
   bool createOutputPort(const std::string & name);
   bool addObject(const std::string &portName, const std::string &objectName);

   MessageQueue *sendMessageQueue;

   const std::string name;
   const int rank;
   const int size;
   const int moduleID;

 private:
   bool handleMessage(const message::Message *message);
   bool addInputObject(const std::string &portName,
                       const std::string &objectName);

   virtual bool compute() = 0;

   MessageQueue *receiveMessageQueue;

   std::map<std::string, std::list<std::string> *> outputPorts;
   std::map<std::string, std::list<std::string> *> inputPorts;
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
