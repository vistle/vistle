#include <iostream>
#include <iomanip>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

using namespace boost::interprocess;

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

void Shm::publish(const shm_handle_t & handle) {

   if (messageQueue) {
      vistle::message::NewObject n(moduleID, rank, handle);
      messageQueue->getMessageQueue().send(&n, sizeof(n), 0);
   }
}

std::string Shm::createObjectID() {

   std::stringstream name;
   name << "Object_" << std::setw(8) << std::setfill('0') << moduleID
        << "_" << std::setw(8) << std::setfill('0') << rank
        << "_" << std::setw(8) << std::setfill('0') << objectID ++;

   return name.str();
}


   /*
template <class T> Vec3<T>::Vec3(const size_t s): size(s) {

   x = Shm::instance().getShm().allocate(s * sizeof(T));
   y = Shm::instance().getShm().allocate(s * sizeof(T));
   z = Shm::instance().getShm().allocate(s * sizeof(T));
}
   */

Object::Object(const Type type): id(type) {

}

Object::~Object() {

}

Object::Type Object::getType() const {

   return id;
}

template<> void Vec<float>::setType()  { id = Object::VECFLOAT; }
template<> void Vec<int>::setType()    { id = Object::VECINT; }
template<> void Vec<char>::setType()   { id = Object::VECCHAR; }

template<> void Vec3<float>::setType() { id = Object::VEC3FLOAT; }
template<> void Vec3<int>::setType()   { id = Object::VEC3INT; }
template<> void Vec3<char>::setType()  { id = Object::VEC3CHAR; }

} // namespace vistle
