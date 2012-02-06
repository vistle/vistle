/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#else
#include<Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#define usleep Sleep
#define close closesocket
typedef int socklen_t;
#endif

#include <mpi.h>

#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>

#include <sstream>
#include <iostream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

#include "vistle.h"

using namespace boost::interprocess;


void spawn(vistle::Communicator * comm, const int rank,
           const int moduleID, const char * name) {

   vistle::message::Spawn module(0, rank, moduleID, name);
   comm->handleMessage(&module);
}

void setParam(vistle::Communicator * comm, const int rank,
             const int moduleID, const char * name, const float value) {

   vistle::message::SetFloatParameter param(0, rank, moduleID, name, value);
   comm->handleMessage(&param);
}

void setParam(vistle::Communicator * comm, const int rank,
             const int moduleID, const char * name, const std::string & value) {

   vistle::message::SetFileParameter param(0, rank, moduleID, name, value);
   comm->handleMessage(&param);
}

void setParam(vistle::Communicator * comm, const int rank,
             const int moduleID, const char * name, const vistle::Vector & value) {

   vistle::message::SetVectorParameter param(0, rank, moduleID, name, value);
   comm->handleMessage(&param);
}

void connect(vistle::Communicator * comm, const int rank,
             const int moduleA, const char * aPort,
             const int moduleB, const char * bPort) {

   vistle::message::Connect connect(0, rank, moduleA, aPort, moduleB, bPort);
   comm->handleMessage(&connect);
}

void compute(vistle::Communicator * comm, const int rank,  const int moduleID) {

   vistle::message::Compute comp(0, rank, moduleID);
   comm->handleMessage(&comp);
}

int main(int argc, char ** argv) {

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

#if 0
   vistle::message::Spawn readCovise1(0, rank, 1, "ReadCovise");
   comm->handleMessage(&readCovise1);

   vistle::message::Spawn readCovise2(0, rank, 2, "ReadCovise");
   comm->handleMessage(&readCovise2);

   vistle::message::Spawn isoSurface(0, rank, 3, "IsoSurface");
   comm->handleMessage(&isoSurface);

   vistle::message::Spawn showUSG(0, rank, 4, "ShowUSG");
   comm->handleMessage(&showUSG);

   vistle::message::Spawn renderer(0, rank, 5, "OSGRenderer");
   comm->handleMessage(&renderer);

   vistle::message::Connect connect13g(0, rank, 1, "grid_out", 3, "grid_in");
   comm->handleMessage(&connect13g);

   vistle::message::Connect connect13d(0, rank, 2, "grid_out", 3, "data_in");
   comm->handleMessage(&connect13d);

   vistle::message::Connect connect14(0, rank, 1, "grid_out", 4, "grid_in");
   comm->handleMessage(&connect14);

   vistle::message::Connect connect45(0, rank, 4, "grid_out", 5, "data_in");
   comm->handleMessage(&connect45);

   vistle::message::Connect connect35(0, rank, 3, "grid_out", 5, "data_in");
   comm->handleMessage(&connect35);

   vistle::message::SetFileParameter param1(0, rank, 1, "filename",
                                           "/tmp/g.covise");
   comm->handleMessage(&param1);

   vistle::message::SetFileParameter param2(0, rank, 2, "filename",
                                           "/tmp/p.covise");
   comm->handleMessage(&param2);

   vistle::message::Compute compute1(0, rank, 1);
   comm->handleMessage(&compute1);

   vistle::message::Compute compute2(0, rank, 2);
   comm->handleMessage(&compute2);
#endif

#if 0
   vistle::message::Spawn readCovise(0, rank, 1, "ReadCovise");
   comm->handleMessage(&readCovise);

   vistle::message::SetFileParameter param(0, rank, 1, "filename",
                                           "/tmp/single_geom2d.covise");
   comm->handleMessage(&param);

   vistle::message::Spawn renderer(0, rank, 2, "OSGRenderer");
   comm->handleMessage(&renderer);

   vistle::message::Connect connect12(0, rank, 1, "grid_out", 2, "data_in");
   comm->handleMessage(&connect12);

   vistle::message::Compute compute(0, rank, 1);
   comm->handleMessage(&compute);
#endif

#if 0
   vistle::message::Spawn gendat(0, rank, 1, "Gendat");
   comm->handleMessage(&gendat);
   vistle::message::Spawn renderer(0, rank, 2, "OSGRenderer");
   comm->handleMessage(&renderer);

   vistle::message::Connect connect(0, rank, 1, "data_out", 2, "data_in");
   comm->handleMessage(&connect);

   vistle::message::Compute compute(0, rank, 1);
   comm->handleMessage(&compute);
#endif

#if 0
   enum { RGEO = 1, RGRID, RPRES, CUTGEO, CUTSURF, ISOSURF, COLOR, COLLECT, RENDERER, WRITEVISTLE };

   spawn(comm, rank, RGEO,  "ReadVistle");
   /*
   spawn(comm, rank, RGRID, "ReadCovise");
   spawn(comm, rank, RPRES, "ReadCovise");

   spawn(comm, rank, CUTGEO, "CutGeometry");
   spawn(comm, rank, CUTSURF, "CuttingSurface");
   spawn(comm, rank, ISOSURF, "IsoSurface");

   spawn(comm, rank, COLOR, "Color");
   spawn(comm, rank, COLLECT, "Collect");
   spawn(comm, rank, WRITEVISTLE, "WriteVistle");
   */
   spawn(comm, rank, RENDERER, "OSGRenderer");

   setParam(comm, rank, RGEO, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/three_geo2d.vistle");

   setParam(comm, rank, RGRID, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/multi_geo3d.covise");

   setParam(comm, rank, RPRES, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/multi_p.covise");
   /*
   setParam(comm, rank, WRITEVISTLE, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/test/three_geo2d.vistle");
   */
   setParam(comm, rank, ISOSURF, "isovalue", -1.0);

   setParam(comm, rank, CUTSURF, "distance", 0.0);
   setParam(comm, rank, CUTSURF, "normal", vistle::Vector(1.0, 0.0, 0.0));

   connect(comm, rank, RGEO, "grid_out", RENDERER, "data_in");
   //connect(comm, rank, CUTGEO, "grid_out", RENDERER, "data_in");

   connect(comm, rank, RGRID, "grid_out", CUTSURF, "grid_in");
   connect(comm, rank, RPRES, "grid_out", CUTSURF, "data_in");
   connect(comm, rank, CUTSURF, "grid_out", COLLECT, "grid_in");
   connect(comm, rank, CUTSURF, "data_out", COLOR, "data_in");
   connect(comm, rank, COLOR, "data_out", COLLECT, "texture_in");
   connect(comm, rank, COLLECT, "grid_out", RENDERER, "data_in");

   connect(comm, rank, RGRID, "grid_out", ISOSURF, "grid_in");
   connect(comm, rank, RPRES, "grid_out", ISOSURF, "data_in");
   connect(comm, rank, ISOSURF, "grid_out", RENDERER, "data_in");

   //connect(comm, rank, RGEO, "grid_out", WRITEVISTLE, "grid_in");

   compute(comm, rank, RGEO);
   compute(comm, rank, RGRID);
   compute(comm, rank, RPRES);
#endif

#if 0
   vistle::message::Spawn readCovise(0, rank, 1, "ReadCovise");
   comm->handleMessage(&readCovise);

   vistle::message::SetFileParameter param(0, rank, 1, "filename",
                     "/data/OpenFOAM/PumpTurbine/covise/p.covise");
   comm->handleMessage(&param);

   vistle::message::Spawn color(0, rank, 2, "Color");
   comm->handleMessage(&color);

   vistle::message::Connect connect12(0, rank, 1, "grid_out", 2, "data_in");
   comm->handleMessage(&connect12);

   vistle::message::Compute compute(0, rank, 1);
   comm->handleMessage(&compute);
#endif

#if 0
   enum { READ = 1, WRITE };
   spawn(comm, rank, READ, "ReadCovise");
   spawn(comm, rank, WRITE, "WriteVistle");

   setParam(comm, rank, READ, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/geo3d.covise");

   setParam(comm, rank, WRITE, "filename",
            "/data/OpenFOAM/PumpTurbine/covise/geo3d.vistle");

   connect(comm, rank, READ, "grid_out", WRITE, "grid_in");

   compute(comm, rank, READ);
#endif

#if 1
   enum { READ = 1, RENDERER };
   spawn(comm, rank, READ, "ReadFOAM");
   spawn(comm, rank, RENDERER, "OSGRenderer");

   setParam(comm, rank, READ, "filename",
            "/data/OpenFOAM/PumpTurbine/transient");

   connect(comm, rank, READ, "grid_out", RENDERER, "data_in");

   compute(comm, rank, READ);
#endif

   while (!done) {
      done = comm->dispatch();
      usleep(100);
   }

   delete comm;

   shared_memory_object::remove("vistle");

   //MPI_Finalize();
}

int acceptClient() {

   int server = socket(AF_INET, SOCK_STREAM, 0);
   int reuse = 1;

   setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse,
              sizeof(reuse));

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
   mpiReceiveBuffer = new char[message::Message::MESSAGE_SIZE];

   // post requests for length of next MPI message
   MPI_Irecv(&mpiMessageSize, 1, MPI_INT, MPI_ANY_SOURCE, 0,
             MPI_COMM_WORLD, &request);
   MPI_Barrier(MPI_COMM_WORLD);
   /*
   if (rank == 0)
      clientSocket = acceptClient();
   */
}

bool Communicator::dispatch() {

   bool done = false;

   if (rank == 0 && clientSocket != -1) {
      // poll socket
      fd_set set;
      FD_ZERO(&set);
      FD_SET(clientSocket, &set);

      struct timeval t = { 0, 0 };

      select(clientSocket + 1, &set, NULL, NULL, &t);

      if (FD_ISSET(clientSocket, &set)) {

         message::Message *message = NULL;

#ifdef _WIN32
         recv(clientSocket, (char *) socketBuffer, 1,0);
#else
         int r = read(clientSocket, socketBuffer, 1);
#endif

         if (r == 1) {

            if (socketBuffer[0] == 'q')
               message = new message::Quit(0, rank);

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
   char msgRecvBuf[vistle::message::Message::MESSAGE_SIZE];

   std::map<int, message::MessageQueue *>::iterator i;
   for (i = receiveMessageQueue.begin(); i != receiveMessageQueue.end(); ){

      bool moduleExit = false;
      try {
         bool received =
            i->second->getMessageQueue().try_receive(
                                        (void *) msgRecvBuf,
                                        vistle::message::Message::MESSAGE_SIZE,
                                        msgSize, priority);

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


bool Communicator::handleMessage(const message::Message * message) {

   switch (message->getType()) {

      case message::Message::DEBUG: {

         const message::Debug *debug =
            static_cast<const message::Debug *>(message);
         std::cout << "comm [" << rank << "/" << size << "] Debug ["
                   << debug->getCharacter() << "]" << std::endl;
         break;
      }

      case message::Message::QUIT: {

         const message::Quit *quit =
            static_cast<const message::Quit *>(message);
         (void) quit;
         return false;
         break;
      }

      case message::Message::SPAWN: {

         const message::Spawn *spawn =
            static_cast<const message::Spawn *>(message);
         int moduleID = spawn->getSpawnID();

         std::stringstream name;
         name << "bin/" << spawn->getName();

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

         MPI_Comm_spawn(strdup(name.str().c_str()), argv, size, MPI_INFO_NULL,
                        0, MPI_COMM_WORLD, &interComm, MPI_ERRCODES_IGNORE);

         break;
      }

      case message::Message::CONNECT: {

         const message::Connect *connect =
            static_cast<const message::Connect *>(message);
         portManager.addConnection(connect->getModuleA(),
                                   connect->getPortAName(),
                                   connect->getModuleB(),
                                   connect->getPortBName());
         break;
      }

      case message::Message::NEWOBJECT: {

         /*
         const message::NewObject *newObject =
            static_cast<const message::NewObject *>(message);
         vistle::Object *object = (vistle::Object *)
            vistle::Shm::instance().getShm().get_address_from_handle(newObject->getHandle());

         std::cout << "comm [" << rank << "/" << size << "] NewObject ["
                   << newObject->getHandle() << "] type ["
                   << object->getType() << "] from module ["
                   << newObject->getModuleID() << "]" << std::endl;
         */
         break;
      }

      case message::Message::MODULEEXIT: {

         const message::ModuleExit *moduleExit =
            static_cast<const message::ModuleExit *>(message);
         int mod = moduleExit->getModuleID();

         std::cout << "comm [" << rank << "/" << size << "] Module ["
                   << mod << "] quit" << std::endl;

         return false;
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute *comp =
            static_cast<const message::Compute *>(message);
         std::map<int, message::MessageQueue *>::iterator i
            = sendMessageQueue.find(comp->getModule());
         if (i != sendMessageQueue.end())
            i->second->getMessageQueue().send(comp, sizeof(*comp), 0);
         break;
      }

      case message::Message::CREATEINPUTPORT: {

         const message::CreateInputPort *m =
            static_cast<const message::CreateInputPort *>(message);
         portManager.addPort(m->getModuleID(), m->getName(),
                             Port::INPUT);
         break;
      }

      case message::Message::CREATEOUTPUTPORT: {

         const message::CreateOutputPort *m =
            static_cast<const message::CreateOutputPort *>(message);
         portManager.addPort(m->getModuleID(), m->getName(),
                             Port::OUTPUT);
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject *m =
            static_cast<const message::AddObject *>(message);
         std::cout << "Module " << m->getModuleID() << ": "
                   << "AddObject " << m->getHandle()
                   << " to port " << m->getPortName() << std::endl;

         Port *port = portManager.getPort(m->getModuleID(),
                                          m->getPortName());
         if (port) {
            port->addObject(m->getHandle());
            const std::vector<const Port *> *list =
               portManager.getConnectionList(port);

            std::vector<const Port *>::const_iterator pi;
            for (pi = list->begin(); pi != list->end(); pi ++) {

               std::map<int, message::MessageQueue *>::iterator mi =
                  sendMessageQueue.find((*pi)->getModuleID());
               if (mi != sendMessageQueue.end()) {
                  const message::AddObject a(m->getModuleID(), m->getRank(),
                                             (*pi)->getName(), m->getHandle());
                  const message::Compute c(moduleID, rank,
                                           (*pi)->getModuleID());

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

      case message::Message::SETFILEPARAMETER: {

         const message::SetFileParameter *m =
            static_cast<const message::SetFileParameter *>(message);

         if (m->getModuleID() != m->getModule()) {
            // message to module
            std::map<int, message::MessageQueue *>::iterator i
               = sendMessageQueue.find(m->getModule());
            if (i != sendMessageQueue.end())
               i->second->getMessageQueue().send(m, m->getSize(), 0);
         }
         break;
      }

      case message::Message::SETFLOATPARAMETER: {

         const message::SetFloatParameter *m =
            static_cast<const message::SetFloatParameter *>(message);

         if (m->getModuleID() != m->getModule()) {
            // message to module
            std::map<int, message::MessageQueue *>::iterator i
               = sendMessageQueue.find(m->getModule());
            if (i != sendMessageQueue.end())
               i->second->getMessageQueue().send(m, m->getSize(), 0);
         }
         break;
      }

      case message::Message::SETINTPARAMETER: {

         const message::SetIntParameter *m =
            static_cast<const message::SetIntParameter *>(message);

         if (m->getModuleID() != m->getModule()) {
            // message to module
            std::map<int, message::MessageQueue *>::iterator i
               = sendMessageQueue.find(m->getModule());
            if (i != sendMessageQueue.end())
               i->second->getMessageQueue().send(m, m->getSize(), 0);
         }
         break;

      }

      case message::Message::SETVECTORPARAMETER: {

         const message::SetVectorParameter *m =
            static_cast<const message::SetVectorParameter *>(message);

         if (m->getModuleID() != m->getModule()) {
            // message to module
            std::map<int, message::MessageQueue *>::iterator i
               = sendMessageQueue.find(m->getModule());
            if (i != sendMessageQueue.end())
               i->second->getMessageQueue().send(m, m->getSize(), 0);
         }
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
      usleep(1000);
   }

   if (clientSocket != -1)
      close(clientSocket);

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
