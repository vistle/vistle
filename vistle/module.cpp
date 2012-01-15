#include <mpi.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "object.h"
#include "message.h"
#include "messagequeue.h"
#include "module.h"

using namespace boost::interprocess;

int main(int argc, char **argv) {

   int rank, size, irank, isize;
   MPI_Init(&argc, &argv);

   if (argc != 2) {

      std::cerr << "module missing parameters" << std::endl;
      exit(1);
   }

   MPI_Comm parentCommunicator;
   MPI_Comm_get_parent(&parentCommunicator);

   MPI_Comm_rank(parentCommunicator, &irank);
   MPI_Comm_size(parentCommunicator, &isize);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);

   int moduleID = atoi(argv[1]);
   vistle::Shm::instance();

   std::cout << "module " << moduleID << " [" << rank << "/" << size
             << "] started" << std::endl;

   std::string smqName =
      vistle::MessageQueue::createName("rmq", moduleID, rank);
   std::string rmqName =
      vistle::MessageQueue::createName("smq", moduleID, rank);

   vistle::MessageQueue *smq, *rmq;
   try {
      smq = vistle::MessageQueue::open(smqName);
      rmq = vistle::MessageQueue::open(rmqName);
   } catch (interprocess_exception &ex) {
      std::cout << "module " << moduleID << " [" << rank << "/" << size << "] "
                << ex.what() << std::endl;
      exit(-1);
   }

   vistle::Module *module = new vistle::Module(rank, size, moduleID, *rmq, *smq);
   (void) module;

   MPI_Finalize();

   std::cout << "module " << moduleID << " [" << rank << "/" << size
             << "] done" << std::endl;

   return 0;
}

namespace vistle {

Module::Module(int r, int s, int m,
               MessageQueue & rm, MessageQueue & sm)
   : rank(r), size(s), moduleID(m),
     receiveMessageQueue(rm), sendMessageQueue(sm) {

   size_t msgSize;
   unsigned int priority;
   char msgRecvBuf[128];

   receiveMessageQueue.getMessageQueue().receive((void *) msgRecvBuf,
                                                 (size_t) 128, msgSize,
                                                 priority);

   vistle::message::Message *message = (vistle::message::Message *) msgRecvBuf;

   switch (message->getType()) {
      case vistle::message::Message::DEBUG: {

         vistle::message::Debug *debug = (vistle::message::Debug *) message;
         std::cout << "module " << moduleID << " [" << rank << "/" << size
                   << "] debug [" << debug->getCharacter() << "]" << std::endl;
         break;
      }
      default:
         break;
   }

   std::stringstream name;
   name << "Object_" << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank;

   try {
      vistle::FloatArray a(name.str());

      vistle::message::NewObject n(name.str());
      sendMessageQueue.getMessageQueue().send(&n, sizeof(n), 0);

      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]"
         " object [" << name.str() << "] allocated" << std::endl;

      for (int index = 0; index < 128; index ++)
         a.vec->push_back(index);
   } catch (interprocess_exception &ex) {
      std::cout << "module " << moduleID << " [" << rank << "/" << size << "]:"
         " object [" << name.str() << "] already exists" << std::endl;
   }

   int *local = new int[1024 * 1024];
   int *global = new int[1024 * 1024];

   for (int count = 0; count < 128; count ++) {
      for (int index = 0; index < 1024 * 1024; index ++)
         local[index] = rand();
      MPI_Allreduce(local, global, 1024 * 1024, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }

   vistle::message::ModuleExit m(moduleID, rank);
   sendMessageQueue.getMessageQueue().send(&m, sizeof(m), 0);
}

} // namespace vistle
