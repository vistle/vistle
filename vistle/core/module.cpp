#include <mpi.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include<Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "object.h"
#include "message.h"
#include "messagequeue.h"
#include "parameter.h"
#include "module.h"

using namespace boost::interprocess;

namespace vistle {

Module::Module(const std::string &n, const int r, const int s, const int m)
   : name(n), rank(r), size(s), moduleID(m) {

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   const int HOSTNAMESIZE = 64;
   char hostname[HOSTNAMESIZE];
   gethostname(hostname, HOSTNAMESIZE - 1);

   std::cout << "  module [" << name << "] [" << moduleID << "] [" << rank
             << "/" << size << "] started as " << hostname << ":"
#ifndef _WIN32
             << getpid() << std::endl;
#else
             << std::endl;
#endif

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

bool Module::addFileParameter(const std::string & name,
                              const std::string & value) {

   std::map<std::string, Parameter *>::iterator i =
      parameters.find(name);

   if (i == parameters.end()) {

      parameters[name] = new FileParameter(name, value);
      message::AddFileParameter message(moduleID, rank, name, value);
      sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);

      return true;
   }
   return false;
}

void Module::setFileParameter(const std::string & name,
                              const std::string & value) {

   std::map<std::string, Parameter *>::iterator i =
      parameters.find(name);

   if (i == parameters.end())
      parameters[name] = new FileParameter(name, value);
   else {
      FileParameter *param = dynamic_cast<FileParameter *>(i->second);
      if (param)
         param->setValue(value);
      else
         return;
   }

   message::SetFileParameter message(moduleID, rank, moduleID, name, value);
   sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);
}

std::string Module::getFileParameter(const std::string & name) const {

  std::map<std::string, Parameter *>::const_iterator i =
      parameters.find(name);

  if (i == parameters.end())
     return "";
   else {
      FileParameter *param = dynamic_cast<FileParameter *>(i->second);
      if (param)
         return param->getValue();
   }
  return "";
}

bool Module::addFloatParameter(const std::string & name,
                               const float value) {

   std::map<std::string, Parameter *>::iterator i =
      parameters.find(name);

   if (i == parameters.end()) {

      parameters[name] = new FloatParameter(name, value);
      message::AddFloatParameter message(moduleID, rank, name, value);
      sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);

      return true;
   }
   return false;
}

void Module::setFloatParameter(const std::string & name,
                               const float value) {

   std::map<std::string, Parameter *>::iterator i =
      parameters.find(name);

   if (i == parameters.end())
      parameters[name] = new FloatParameter(name, value);
   else {
      FloatParameter *param = dynamic_cast<FloatParameter *>(i->second);
      if (param)
         param->setValue(value);
      else
         return;
   }

   message::SetFloatParameter message(moduleID, rank, moduleID, name, value);
   sendMessageQueue->getMessageQueue().send(&message, sizeof(message), 0);
}

float Module::getFloatParameter(const std::string & name) const {

  std::map<std::string, Parameter *>::const_iterator i =
      parameters.find(name);

  if (i == parameters.end())
     return 0.0;
   else {
      FloatParameter *param = dynamic_cast<FloatParameter *>(i->second);
      if (param)
         return param->getValue();
   }
  return 0.0;
}

bool Module::addObject(const std::string & portName,
                       const shm_handle_t & handle) {
   /*
   std::cout << "Module::addObject " << handle << " to port " <<  portName
             << std::endl;
   */
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

bool Module::addObject(const std::string & portName, const void *object) {

   if (!object)
      return false;

   boost::interprocess::managed_shared_memory::handle_t handle =
      vistle::Shm::instance().getShm().get_handle_from_address(object);

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
      for (shmit = i->second->begin(); shmit != i->second->end(); ) {
         Object *o = Shm::instance().getObjectFromHandle(*shmit);
         if (object == o)
            shmit = i->second->erase(shmit);
         else
            shmit ++;
      }
   }
}

bool Module::addInputObject(const std::string & portName,
                            const shm_handle_t & handle) {

   std::map<std::string, std::list<shm_handle_t> *>::iterator i =
      inputPorts.find(portName);

   if (i != inputPorts.end()) {
      i->second->push_back(handle);
      return addInputObject(portName,
                            Shm::instance().getObjectFromHandle(handle));
   }
   return false;
}

bool Module::addInputObject(const std::string & portName,
                            const Object * object) {

   return true;
}

bool Module::dispatch() {

   size_t msgSize;
   unsigned int priority;
   char msgRecvBuf[message::Message::MESSAGE_SIZE];

   receiveMessageQueue->getMessageQueue().receive(
                                               (void *) msgRecvBuf,
                                               message::Message::MESSAGE_SIZE,
                                               msgSize, priority);

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

      case message::Message::SETFILEPARAMETER: {

         const message::SetFileParameter *param =
            static_cast<const message::SetFileParameter *>(message);

         setFileParameter(param->getName(), param->getValue());
         break;
      }

      case message::Message::SETFLOATPARAMETER: {

         const message::SetFloatParameter *param =
            static_cast<const message::SetFloatParameter *>(message);

         setFloatParameter(param->getName(), param->getValue());
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
