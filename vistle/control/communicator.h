#ifndef COMMUNICATOR_COLLECTIVE_H
#define COMMUNICATOR_COLLECTIVE_H

#include <vector>
#include <deque>
#include <map>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "portmanager.h"

namespace bi = boost::interprocess;

namespace vistle {

namespace message {
   struct Message;
   class MessageQueue;
}

class Communicator {

 public:
   Communicator(int argc, char *argv[], int rank, int size);
   ~Communicator();

   bool dispatch();
   bool handleMessage(const message::Message &message);

   int getRank() const;
   int getSize() const;

 private:

   std::string m_bindir;

   const int rank;
   const int size;

   std::vector<std::deque<char> > readbuf, writebuf;
   std::vector<int> sockfd;
   int moduleID;

   char * mpiReceiveBuffer;
   int mpiMessageSize;

   MPI_Request request;

   std::map<int, message::MessageQueue *> sendMessageQueue;
   std::map<int, message::MessageQueue *> receiveMessageQueue;

   std::map<int, bi::shared_memory_object *> shmObjects;

   PortManager portManager;

   int checkClients();
   void flushClient(int num);
   void writeClient(int num, const void *buf, size_t n);
   void writeClient(int num, const std::string &s);
   void disconnectClients();
};

} // namespace vistle

#endif
