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
#include <iostream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

#include "communicator_collective.h"

using namespace boost::interprocess;

int main(int argc, char **argv) {

   MPI_Init(&argc, &argv);

   int rank, size;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   // process with the smallest rank on each host allocates shm
   const int HOSTNAMESIZE = 64;

   char hostname[HOSTNAMESIZE];
   char *hostnames = new char[HOSTNAMESIZE * size];
   gethostname(hostname, HOSTNAMESIZE - 1);

   MPI_Allgather(hostname, HOSTNAMESIZE, MPI_CHAR,
                 hostnames, HOSTNAMESIZE, MPI_CHAR, MPI_COMM_WORLD);

   bool first = true;
   for (int index = 0; index < rank; index ++)
      if (!strncmp(hostname, hostnames + index * HOSTNAMESIZE, HOSTNAMESIZE))
         first = false;

   if (first) {
      shared_memory_object::remove("vistle");
      vistle::Shm::instance();
   }
   MPI_Barrier(MPI_COMM_WORLD);

   if (!first)
      vistle::Shm::instance();
   MPI_Barrier(MPI_COMM_WORLD);

   vistle::Communicator *comm = new vistle::Communicator(rank, size);
   bool done = false;

   while (!done) {
      done = comm->dispatch();
      usleep(1000);
   }

   delete comm;

   shared_memory_object::remove("vistle");
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

Communicator::Communicator(int r, int s)
   : rank(r), size(s), socketBuffer(NULL), clientSocket(-1), moduleID(0),
     mpiReceiveBuffer(NULL), mpiMessageSize(0) {

   socketBuffer = new unsigned char[64];
   mpiReceiveBuffer = new char[1024];

   // post requests for length of next MPI message
   MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, 0,
             MPI_COMM_WORLD, &request); 
   MPI_Barrier(MPI_COMM_WORLD);

   if (rank == 0)
      clientSocket = acceptClient();

   std::stringstream name;
   name << "Object_" << std::setw(8) << std::setfill('0') << 0
        << "_" << std::setw(8) << std::setfill('0') << rank;

   try {
      vistle::FloatArray a(name.str());
      std::cout << "rank " << rank << ": object [" << name.str()
                << "] allocated" << std::endl;

      for (int index = 0; index < 128; index ++)
         a.vec->push_back(index);
   } catch (interprocess_exception &ex) {
      std::cerr << "rank " << rank << ": object " << name.str()
                << " already exists" << std::endl;
   }
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

         // Broadcast message to other MPI partitions
         //   - handle message, delete message
         if (message) {

            MPI_Request s;
            for (int index = 0; index < size; index ++)
               if (index != rank)
                  MPI_Isend(&(message->size), 1, MPI_INT, index, 0,
                            MPI_COMM_WORLD, &s);

            MPI_Bcast(message, message->size, MPI_BYTE, 0, MPI_COMM_WORLD);

            if (!handleMessage(message))
               done = true;

            delete message;
         }
      }
   }

   int flag;
   MPI_Status status;

   // test for message size from another MPI node
   //    - receive actual message from broadcast
   //    - handle message
   //    - post another MPI receive for size of next message
   MPI_Test(&request, &flag, &status);

   if (flag) {
      if (status.MPI_TAG == 0) {

         MPI_Bcast(mpiReceiveBuffer, mpiMessageSize, MPI_BYTE,
                   status.MPI_SOURCE, MPI_COMM_WORLD);

         message::Message *message = (message::Message *) mpiReceiveBuffer;
         printf("[%02d] message from [%02d] message type %d size %d\n",
                rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);

         if (!handleMessage(message))
            done = true;

         MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, 0,
                   MPI_COMM_WORLD, &request);
      }
   }

   // test for messages from modules
   size_t msgSize;
   unsigned int priority;
   char msgRecvBuf[128];

   std::map<int, MessageQueue *>::iterator i;
   for (i = receiveMessageQueue.begin(); i != receiveMessageQueue.end(); i ++) {

      try {
         bool received = 
            i->second->getMessageQueue().try_receive((void *) msgRecvBuf,
                                                     (size_t) 128, msgSize,
                                                     priority);

         if (received)
            handleMessage((message::Message *) msgRecvBuf);

      } catch (boost::interprocess::interprocess_exception &ex) {
         std::cerr << "comm [" << rank << "/" << size << "] receive mq "
                   << ex.what() << std::endl;
         exit(-1);
      }
   }

   return done;
}


bool Communicator::handleMessage(message::Message *message) {

   switch (message->getType()) {

      case message::Message::DEBUG: {

         message::Debug *debug = (message::Debug *) message;
         std::cout << "comm [" << rank << "/" << size << "] Debug ["
                   << debug->getCharacter() << "]" << std::endl;
         break;
      }

      case message::Message::QUIT: {

         message::Quit *quit = (message::Quit *) message;
         (void) quit;
         return false;
         break;
      }

      case message::Message::SPAWN: {

         message::Spawn *spawn = (message::Spawn *) message;
         int moduleID = spawn->getModuleID();

         std::stringstream modID;
         modID << moduleID;

         std::string smqName = MessageQueue::createName("smq", moduleID, rank);
         std::string rmqName = MessageQueue::createName("rmq", moduleID, rank);

         try {
            MessageQueue *smq = MessageQueue::create(smqName);
            sendMessageQueue[moduleID] = smq;

            MessageQueue *rmq = MessageQueue::create(rmqName);
            receiveMessageQueue[moduleID] = rmq;

            message::Debug d('A');
            smq->getMessageQueue().send(&d, sizeof(d), 0);
         } catch (boost::interprocess::interprocess_exception &ex) {
            std::cerr << "comm [" << rank << "/" << size << "] spawn mq "
                      << ex.what() << std::endl;
            exit(-1);
         }

         MPI_Comm interComm;
         char *argv[2] = { strdup(modID.str().c_str()), NULL };

         MPI_Comm_spawn((char *) "module", argv, size, MPI_INFO_NULL, 0,
                        MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);

         break;
      }

      case message::Message::NEWOBJECT: {

         message::NewObject *newObject = (message::NewObject *) message;
         std::cout << "comm [" << rank << "/" << size << "] NewObject ["
                   << newObject->getName() << "]" << std::endl;
         break;
      }

      case message::Message::MODULEEXIT: {

         message::ModuleExit *moduleExit = (message::ModuleExit *) message;
         int mod = moduleExit->getModuleID();

         std::cout << "comm [" << rank << "/" << size << "] Module ["
                   << mod << "] quit" << std::endl;

         std::map<int, MessageQueue *>::iterator i = sendMessageQueue.find(mod);
         if (i != sendMessageQueue.end()) {
            delete i->second;
            sendMessageQueue.erase(i);
         }

         i = receiveMessageQueue.find(mod);
         if (i != receiveMessageQueue.end()) {
            delete i->second;
            receiveMessageQueue.erase(i);
         }
      }
      default:
         break;
   }

   return true;
}

Communicator::~Communicator() {

   if (clientSocket != -1)
      close(clientSocket);
}

} // namespace vistle
