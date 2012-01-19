/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#else
#include<Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif
#include <mpi.h>

#include <sys/types.h>

#ifndef _WIN32
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <sstream>
#include <iostream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

#include "communicator_collective.h"

using namespace boost::interprocess;

int GENDAT = -1;
int ADD = -1;

int main(int argc, char **argv) {

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
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
      vistle::Shm::instance(0, rank, NULL);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   if (!first)
      vistle::Shm::instance(0, rank, NULL);
   MPI_Barrier(MPI_COMM_WORLD);

   vistle::Communicator *comm = new vistle::Communicator(rank, size);
   bool done = false;

   while (!done) {
      done = comm->dispatch();
#ifdef WIN32
      Sleep(100);
#else
      usleep(100);
#endif
   }

   delete comm;

   shared_memory_object::remove("vistle");

   //MPI_Finalize();
}

int acceptClient() {

   int server = socket(AF_INET, SOCK_STREAM, 0);
   int reuse = 1;
#ifdef _WIN32
   setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
#else
   setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

   struct sockaddr_in serv, addr;
   serv.sin_family = AF_INET;
   serv.sin_addr.s_addr = INADDR_ANY;
   serv.sin_port = htons(8192);

   if (bind(server, (struct sockaddr *) &serv, sizeof(serv)) < 0) {
      perror("bind error");
      exit(1);
   }
   listen(server, 0);

#ifdef _WIN32
   int len = sizeof(addr);
   int client = accept(server, (struct sockaddr *) &addr, &len);

   closesocket(server);
#else
   socklen_t len = sizeof(addr);
   int client = accept(server, (struct sockaddr *) &addr, &len);

   close(server);
#endif
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

#ifdef _WIN32
         recv(clientSocket, (char *)socketBuffer, 1,0);
#else
         read(clientSocket, socketBuffer, 1);
#endif

         if (socketBuffer[0] == 'q')
            message = new message::Quit(0, rank);

         else if (socketBuffer[0] == 'g') {
            moduleID++;
            message = new message::Spawn(0, rank, moduleID, "gendat");
            GENDAT = moduleID;
         }

         else if (socketBuffer[0] == 'a') {
            moduleID++;
            message = new message::Spawn(0, rank, moduleID, "add");
            ADD = moduleID;
         }

         else if (socketBuffer[0] == 'c') {
            message = new message::Connect(0, rank, GENDAT, "data_out", ADD, "data_in");
         }

         else if (socketBuffer[0] == 'e') {
            message = new message::Compute(0, rank, GENDAT);
         }

         else if (socketBuffer[0] != '\r' && socketBuffer[0] != '\n')
            message = new message::Debug(0, rank, socketBuffer[0]);

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
#if 0
         printf("[%02d] message from [%02d] message type %d size %d\n",
                rank, status.MPI_SOURCE, message->getType(), mpiMessageSize);
#endif
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

   std::map<int, message::MessageQueue *>::iterator i;
   for (i = receiveMessageQueue.begin(); i != receiveMessageQueue.end(); ){

      bool moduleExit = false;
      try {
         bool received =
            i->second->getMessageQueue().try_receive((void *) msgRecvBuf,
                                                     (size_t) 128, msgSize,
                                                     priority);

         if (received) {
            moduleExit = !handleMessage((message::Message *) msgRecvBuf);

            if (moduleExit) {

               std::map<int, message::MessageQueue *>::iterator si =
                  sendMessageQueue.find(i->first);
               if (si != sendMessageQueue.end()) {
                  delete si->second;
                  sendMessageQueue.erase(si);
               }
               receiveMessageQueue.erase(i++);
            }
         }
      } catch (boost::interprocess::interprocess_exception &ex) {
         std::cerr << "comm [" << rank << "/" << size << "] receive mq "
                   << ex.what() << std::endl;
         exit(-1);
      }
      if (!moduleExit)
         i++;
   }

   return done;
}


bool Communicator::handleMessage(const message::Message *message) {

   switch (message->getType()) {

      case message::Message::DEBUG: {

         const message::Debug *debug = (const message::Debug *) message;
         std::cout << "comm [" << rank << "/" << size << "] Debug ["
                   << debug->getCharacter() << "]" << std::endl;
         break;
      }

      case message::Message::QUIT: {

         const message::Quit *quit = (const message::Quit *) message;
         (void) quit;
         return false;
         break;
      }

      case message::Message::SPAWN: {

         const message::Spawn *spawn = (const message::Spawn *) message;
         int moduleID = spawn->getSpawnID();
         const char *name = spawn->getName();

         std::stringstream modID;
         modID << moduleID;

         std::string smqName =
            message::MessageQueue::createName("smq", moduleID, rank);
         std::string rmqName =
            message::MessageQueue::createName("rmq", moduleID, rank);

         try {
            sendMessageQueue[moduleID] =
               message::MessageQueue::create(smqName);
            receiveMessageQueue[moduleID] =
               message::MessageQueue::create(rmqName);
         } catch (boost::interprocess::interprocess_exception &ex) {

            std::cerr << "comm [" << rank << "/" << size << "] spawn mq "
                      << ex.what() << std::endl;
            exit(-1);
         }

         MPI_Comm interComm;
         char *argv[2] = { strdup(modID.str().c_str()), NULL };

         MPI_Comm_spawn(strdup(name), argv, size, MPI_INFO_NULL, 0,
                        MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);

         break;
      }

      case message::Message::CONNECT: {

         const message::Connect *connect = (const message::Connect *) message;
         portManager.addConnection(connect->getModuleA(), connect->getPortAName(),
                                   connect->getModuleB(), connect->getPortBName());
         break;
      }

      case message::Message::NEWOBJECT: {

         const message::NewObject *newObject = (const message::NewObject *) message;

         vistle::Object *object = (vistle::Object *)
            vistle::Shm::instance().getShm().get_address_from_handle(newObject->getHandle());

         std::cout << "comm [" << rank << "/" << size << "] NewObject ["
                   << newObject->getHandle() << "] type ["
                   << object->getType() << "] from module ["
                   << newObject->getModuleID() << "]" << std::endl;

         switch (object->getType()) {

            case Object::VECFLOAT: {
               vistle::Vec<float> *array = static_cast<vistle::Vec<float> *>(object);
               std::cout << "Vec<float> size " << array->getSize() << std::endl;
               /*
               for (unsigned int index = 0; index < array->getSize(); index ++)
                  std::cout << " " << array->x[index];
               std::cout << std::endl;
               */
               break;
            }

            case Object::VEC3INT: {
               vistle::Vec3<int> *array = static_cast<vistle::Vec3<int> *>(object);
               std::cout << "Vec3<int> size " << array->getSize() << std::endl;

               for (unsigned int index = 0; index < array->getSize(); index ++)
                  std::cout << " (" << array->x[index] << " " << array->y[index]
                            << " " << array->z[index] << ")";
               std::cout << std::endl;

               break;
            }

            default:
               std::cout << "unknown data object" << std::endl;
               break;
         }

         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit *moduleExit = (const message::ModuleExit *) message;
         int mod = moduleExit->getModuleID();

         std::cout << "comm [" << rank << "/" << size << "] Module ["
                   << mod << "] quit" << std::endl;

         return false;
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute *comp = (const message::Compute *) message;
         std::map<int, message::MessageQueue *>::iterator i
            = sendMessageQueue.find(comp->getModule());
         if (i != sendMessageQueue.end())
            i->second->getMessageQueue().send(comp, sizeof(*comp), 0);
         break;
      }

      case message::Message::CREATEINPUTPORT: {

         const message::CreateInputPort *m = (const message::CreateInputPort *) message;
         portManager.addPort(m->getModuleID(), m->getName(),
                             Port::INPUT);
         break;
      }

      case message::Message::CREATEOUTPUTPORT: {

         message::CreateOutputPort *m = (message::CreateOutputPort *) message;
         portManager.addPort(m->getModuleID(), m->getName(),
                             Port::OUTPUT);
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject *m = (const message::AddObject *) message;
         std::cout << "AddObject " << m->getHandle()
                   << " to port " << m->getPortName() << std::endl;

         Port *port = portManager.getPort(m->getModuleID(),
                                          m->getPortName());
         if (port) {
            port->addObject(m->getHandle());
            const std::vector<const Port *> *list = portManager.getConnectionList(port);

            std::vector<const Port *>::const_iterator pi;
            for (pi = list->begin(); pi != list->end(); pi ++) {

               std::map<int, message::MessageQueue *>::iterator mi =
                  sendMessageQueue.find((*pi)->getModuleID());
               if (mi != sendMessageQueue.end()) {
                  const message::AddObject a(m->getModuleID(), m->getRank(),
                                             (*pi)->getName(), m->getHandle());
                  const message::Compute c(moduleID, rank, (*pi)->getModuleID());

                  mi->second->getMessageQueue().send(&a, sizeof(a), 0);
                  mi->second->getMessageQueue().send(&c, sizeof(c), 0);
               }
            }
         }
         else
            std::cout << "comm [" << rank << "/" << size << "] Addbject ["
                      << m->getHandle() << "] to port ["
                      << m->getPortName() << "]: port not found" << std::endl;

         break;
      }

      default:
         break;

   }

   return true;
}

Communicator::~Communicator() {

   message::Quit quit(0, rank);
   std::map<int, message::MessageQueue *>::iterator i;

   for (i = sendMessageQueue.begin(); i != sendMessageQueue.end(); i ++)
      i->second->getMessageQueue().send(&quit, sizeof(quit), 1);

   // receive all ModuleExit messages from modules
   // retry for some time, modules that don't answer might have crashed
   int retries = 10000;
   while (sendMessageQueue.size() > 0 && --retries >= 0) {
      dispatch();
#ifdef _WIN32
      Sleep(1000);
#else
      usleep(1000);
#endif
   }

   if (clientSocket != -1)
   {
#ifdef _WIN32
      closesocket(clientSocket);
#else
      close(clientSocket);
#endif
   }

   if (size > 1) {
      int dummy;
      MPI_Request s;
      MPI_Isend(&dummy, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD, &s);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
      MPI_Wait(&s, MPI_STATUS_IGNORE);
   }
   MPI_Barrier(MPI_COMM_WORLD);
}

} // namespace vistle
