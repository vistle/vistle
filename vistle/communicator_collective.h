#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <list>
#include <vector>
#include <map>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace vistle {

namespace message {
   class Message;
}

class MessageQueue;

class Communicator {

 public:
   Communicator(int rank, int size);
   ~Communicator();

   bool dispatch();

 private:
   bool handleMessage(message::Message *message);

   const int rank;
   const int size;

   unsigned char *socketBuffer;
   int clientSocket;
   int moduleID;

   char *mpiReceiveBuffer;
   int mpiMessageSize;

   MPI_Request request;

   std::map<int, MessageQueue *> sendMessageQueue;
   std::map<int, MessageQueue *> receiveMessageQueue;
};

} // namespace vistle

#endif
