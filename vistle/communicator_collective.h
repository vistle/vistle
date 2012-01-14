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
 
class Communicator {
   
 public:
   Communicator(int rank, int size);
   ~Communicator();
   
   bool dispatch();
   
 private:
   bool handleMessage(message::Message *message);
   
   unsigned char *socketBuffer;
   int clientSocket;
   int moduleID;
   
   int rank;
   int size;
   
   char *mpiReceiveBuffer;
   int mpiMessageSize;

   MPI_Request request;
};

} // namespace vistle

#endif
