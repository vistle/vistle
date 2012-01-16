#include <iostream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

namespace vistle {

Shm* Shm::singleton = NULL;

Shm::Shm(const int m, const int r, const size_t &size,
         message::MessageQueue *mq)
   : moduleID(m), rank(r), objectID(0), messageQueue(mq) {

   shm = new managed_shared_memory(open_or_create, "vistle", size);
}

Shm::~Shm() {

}

Shm & Shm::instance(const int moduleID, const int rank,
                    message::MessageQueue *mq) {

   if (!singleton)
      singleton = new Shm(moduleID, rank, 2147483647, mq);

   return *singleton;
}

Shm & Shm::instance() {

   if (!singleton)
      singleton = new Shm(-1, -1, 2147483647, NULL);//34359738368); // 32GB

   return *singleton;
}

managed_shared_memory & Shm::getShm() {

   return *shm;
}

void Shm::publish(const std::string &name) {

   if (messageQueue) {
      vistle::message::NewObject n(moduleID, rank, name);
      messageQueue->getMessageQueue().send(&n, sizeof(n), 0);
   }
}

std::string Shm::createObjectID() {

   std::stringstream name;
   name << "Object_" << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank;

   return name.str();
}

Object::Object() {

}

Object::~Object() {

}

FloatArray::FloatArray(const std::string &name): Object() {

   const FloatShmAllocator
      alloc_inst(Shm::instance().getShm().get_segment_manager());

   /*
   if (Shm::instance().getShm().find<FloatVector>(name.c_str()).first != NULL)
      std::cerr << "object [" << name << "] already exists" << std::endl;
   */
   vec = Shm::instance().getShm().construct<FloatVector>
      (name.c_str())(alloc_inst);

   Shm::instance().publish(name);
}

} // namespace vistle
