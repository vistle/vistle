/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#include <mpi.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sstream>

#include "communicator.h"
#include "message.h"
#include "object.h"

using namespace boost::interprocess;

int main(int argc, char **argv) {

   printf("comm\n");
   MPI_Init(&argc, &argv);

   shared_memory_object::remove("vistle");
   vistle::Shm::instance();
   vistle::FloatArray a;

   for (int index = 0; index < 128; index ++)
      a.vec->push_back(index);

   vistle::Communicator *comm = new vistle::Communicator();
   bool done = false;

   while (!done) {
      done = comm->dispatch();
      usleep(1000);
   }

   delete comm;

   MPI_Finalize();
}

int acceptClient() {

   int server = socket(AF_INET, SOCK_STREAM, 0);
   int reuse = 1;
   setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
   
   struct sockaddr_in serv, addr;
   serv.sin_family = AF_INET;
   serv.sin_addr.s_addr = INADDR_ANY;
   serv.sin_port = htons(8192);
   
   if (bind(server, (struct sockaddr *) &serv, sizeof(serv)) < 0) {
      perror("bind error");
      exit(1);
   }
   listen(server, 0);
   
   socklen_t len = sizeof(addr);
   int client = accept(server, (struct sockaddr *) &addr, &len);
   
   close(server);
   return client;
} 

namespace vistle {

Communicator::Communicator(): socketBuffer(0),
                              clientSocket(-1),
                              moduleID(0) {
   
   socketBuffer = new unsigned char[64];
   memset(socketBuffer, 0, 64);
   
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   rbuf = new char[1024];
   
   // post requests for length of next MPI message
   MPI_Irecv(&messageLength, 1, MPI_INT, MPI_ANY_SOURCE, 0,
             MPI_COMM_WORLD, &request); 
   MPI_Barrier(MPI_COMM_WORLD);
   
   if (rank == 0)
      clientSocket = acceptClient();
}
      
bool Communicator::dispatch() {
   
   bool done = false;
   
   if (rank == 0) {
      
      // poll socket
      fd_set set;
      FD_ZERO(&set);
      FD_SET(clientSocket, &set);
      
      struct timeval t = { 0, 0 };
      
      select(clientSocket + 1, &set, NULL, NULL, &t);
      
      if (FD_ISSET(clientSocket, &set)) {
   
         message::Message *message = NULL;
      
         read(clientSocket, socketBuffer, 1);

         if (socketBuffer[0] == 'q')
            message = new message::Quit();
         
         else if (socketBuffer[0] == 's') {
            moduleID++;
            message = new message::Spawn(moduleID);
         }

         else if (socketBuffer[0] != '\r' && socketBuffer[0] != '\n')
            message = new message::Debug(socketBuffer[0]);

         // TODO: delete message when received by all MPI nodes
         if (message) {

            for (int index = 0; index < size; index ++)
               if (index != rank) {
                  MPI_Request s, r;
                  MPI_Isend(&(message->size), 1, MPI_INT, index, 0,
                            MPI_COMM_WORLD, &s);
                  MPI_Isend(message, message->size, MPI_BYTE, index, 0,
                            MPI_COMM_WORLD, &r);
                  sendRequests.push_back(s);
                  sendRequests.push_back(r);
               }
            sendMessages.push_back(message);

            if (!handleMessage(message))
               done = true;
         }
      }
   }

   int flag;
   int index;
   MPI_Status status;
   
   // test for messages from another MPI node
   //    - handle messages
   //    - post another MPI receive for length of next message
   MPI_Test(&request, &flag, &status);

   if (flag) {
      if (status.MPI_TAG == 0) {
         MPI_Recv(rbuf, messageLength, MPI_BYTE, status.MPI_SOURCE, 0,
                  MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         
         message::Message *message = (message::Message *) rbuf;
         printf("[%02d] message from [%02d] message type %d size %d\n", rank, status.MPI_SOURCE, message->getType(), messageLength);
         
         if (!handleMessage(message))
            done = true;
         
         MPI_Irecv(&messageLength, 1, MPI_INT, MPI_ANY_SOURCE, 0,
                   MPI_COMM_WORLD, &request);
      }
   }
   
   // finish sent MPI requests if they are completed
   do {
      if (sendRequests.size()) {
         MPI_Testany(sendRequests.size(), &(sendRequests[0]), &index,
                     &flag, &status);
         if (flag)
            sendRequests.erase(sendRequests.begin() + index);
      } else
         flag = 0;
   } while (flag);
   
   return done;
}


bool Communicator::handleMessage(message::Message *message) {
   
   switch (message->getType()) {
      
      case message::Message::DEBUG: {

         message::Debug *debug = (message::Debug *) message;
         printf("rank %d debug %d\n", rank, debug->getCharacter());
         break;
      }
         
      case message::Message::QUIT: {

         //message::Quit *quit = (message::Quit *) message;
         return false;
         break;
      }
         
      case message::Message::SPAWN: {
         
         message::Spawn *spawn = (message::Spawn *) message;
         int moduleID = spawn->getModuleID();
         
         std::stringstream modID, shmID;
         modID << moduleID;
         shmID << "vistleSHM_" << moduleID;
         char *shmStr = strdup(shmID.str().c_str());

         MPI_Comm interComm;
         /*
         shared_memory_object *shm =
            new shared_memory_object(open_or_create, shmStr, read_write);
         shm->truncate(16);
         mapped_region region(*shm, read_write);
         std::memset(region.get_address(), 1, region.get_size());
         shmObjects[moduleID] = shm;
         */
         char *argv[3] = { strdup(modID.str().c_str()),
                           shmStr, NULL };
         MPI_Comm_spawn("module", argv, size, MPI_INFO_NULL, 0,
                        MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);

         break;
      }
         
      default:
         break;
   }

   return true;
}

Communicator::~Communicator() {

   if (clientSocket != -1)
      close(clientSocket);

   // remove shared memory objects
   std::map<int, shared_memory_object *>::iterator i;
   for (i = shmObjects.begin(); i != shmObjects.end(); i++)
      shared_memory_object::remove(i->second->get_name());
}

} // namespace vistle
