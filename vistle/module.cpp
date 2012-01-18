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

namespace vistle {

Module::Module(const std::string &n, const int r, const int s, const int m)
   : name(n), rank(r), size(s), moduleID(m) {

   const int HOSTNAMESIZE = 64;

   char hostname[HOSTNAMESIZE];
   gethostname(hostname, HOSTNAMESIZE - 1);

   std::cout << "  module [" << name << "] [" << moduleID << "] [" << rank
             << "/" << size << "] started as " 
             << hostname << ":" << getpid() << std::endl;

   std::string smqName =
      message::MessageQueue::createName("rmq", moduleID, rank);
   std::string rmqName =
      message::MessageQueue::createName("smq", moduleID, rank);

   try {
      sendMessageQueue = message::MessageQueue::open(smqName);
      receiveMessageQueue = message::MessageQueue::open(rmqName);

      Shm::instance(moduleID, rank, sendMessageQueue);

   } catch (interprocess_exception &ex) {
      std::cout << "module " << moduleID << " [" << rank << "/" << size << "] "
                << ex.what() << std::endl;
      exit(2);
   }
}

bool Module::createInputPort(const std::string &name) {

   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      inputPorts.find(name);

   if (i == inputPorts.end()) {

      inputPorts[name] = new std::list<shm_handle_t>;

      message::CreateInputPort message(moduleID, rank, name);
      sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);
      return true;
   }

   return false;
}

bool Module::createOutputPort(const std::string &name) {

   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      outputPorts.find(name);

   if (i == outputPorts.end()) {

      outputPorts[name] = new std::list<shm_handle_t>;

      message::CreateOutputPort message(moduleID, rank, name);
      sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);
      return true;
   }
   return false;
}

bool Module::addObject(const std::string & portName,
                       const shm_handle_t & handle) {

   printf("Module::addObject [%ld] to port [%s]\n", handle, portName.c_str());
   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      outputPorts.find(portName);

   if (i != outputPorts.end()) {
      i->second->push_back(handle);
      message::AddObject message(moduleID, rank, portName, handle);
      sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);
      return true;
   }
   return false;
}

bool Module::addObject(const std::string & portName, const void *p) {

   boost::interprocess::managed_shared_memory::handle_t handle =
      vistle::Shm::instance().getShm().get_handle_from_address(p);

   return addObject(portName, handle);
}

std::list<vistle::Object *> Module::getObjects(const std::string &portName) {

   std::list<vistle::Object *> objects;
   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      inputPorts.find(portName);

   if (i != inputPorts.end()) {

      std::list<shm_handle_t>::iterator shmit;
      for (shmit = i->second->begin(); shmit != i->second->end(); shmit++) {
         Object *object = Shm::instance().getObjectFromHandle(*shmit);
         if (object)
            objects.push_back(object);
      }
   }
   return objects;
}

void Module::removeObject(const std::string &portName, vistle::Object *object) {

   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      inputPorts.find(portName);

   if (i != inputPorts.end()) {

      std::list<shm_handle_t>::iterator shmit;
      for (shmit = i->second->begin(); shmit != i->second->end(); shmit++) {
         Object *o = Shm::instance().getObjectFromHandle(*shmit);
         if (object == o)
            shmit = i->second->erase(shmit);
      }
   }
}

bool Module::addInputObject(const std::string & portName,
                            const shm_handle_t & handle) {

   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      inputPorts.find(portName);

   if (i != inputPorts.end()) {
      printf("          added %ld to [%s]\n", handle, portName.c_str());
      i->second->push_back(handle);
      return true;
   }
   return false;
}

bool Module::dispatch() {

   size_t msgSize;
   unsigned int priority;
   char msgRecvBuf[128];

   receiveMessageQueue->getMessageQueue().receive((void *) msgRecvBuf,
                                                  (size_t) 128, msgSize,
                                                  priority);

   vistle::message::Message *message = (vistle::message::Message *) msgRecvBuf;

   bool done = handleMessage(message);
   if (done) {
      vistle::message::ModuleExit m(moduleID, rank);
      sendMessageQueue->getMessageQueue().send(&m, sizeof(m), 0);
   }

   return done;
}

bool Module::handleMessage(const vistle::message::Message *message) {

   switch (message->getType()) {

      case vistle::message::Message::DEBUG: {

         const vistle::message::Debug *debug =
            static_cast<const vistle::message::Debug *>(message);

         std::cout << "    module [" << name << "] [" << moduleID << "] ["
                   << rank << "/" << size << "] debug ["
                   << debug->getCharacter() << "]" << std::endl;
         break;
      }

      case message::Message::QUIT: {

         const message::Quit *quit =
            static_cast<const message::Quit *>(message);
         (void) quit;
         return true;
         break;
      }

      case message::Message::COMPUTE: {

         const message::Compute *comp =
            static_cast<const message::Compute *>(message);
         (void) comp;
         std::cout << "    module [" << name << "] [" << moduleID << "] ["
                   << rank << "/" << size << "] compute" << std::endl;

         compute();
         break;
      }

      case message::Message::ADDOBJECT: {

         const message::AddObject *add =
            static_cast<const message::AddObject *>(message);
         addInputObject(add->getPortName(), add->getHandle());
         break;
      }

      default:
         std::cout << "    module [" << name << "] [" << moduleID << "] ["
                   << rank << "/" << size << "] unknown message type ["
                   << message->getType() << "]" << std::endl;

         break;
   }

   return false;
}

Module::~Module() {

   std::cout << "  module [" << name << "] [" << moduleID << "] [" << rank
             << "/" << size << "] quit" << std::endl;

   MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();
}

} // namespace vistle
