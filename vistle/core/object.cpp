#include <iostream>
#include <iomanip>

#include <limits.h>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

using namespace boost::interprocess;

namespace vistle {

Shm* Shm::singleton = NULL;

Shm::Shm(const int m, const int r, const size_t & size,
         message::MessageQueue * mq)
   : moduleID(m), rank(r), objectID(0), messageQueue(mq) {

   shm = new managed_shared_memory(open_or_create, "vistle", size);
}

Shm::~Shm() {

   delete shm;
}

Shm & Shm::instance(const int moduleID, const int rank,
                    message::MessageQueue * mq) {

   if (!singleton)
      singleton = new Shm(moduleID, rank, INT_MAX, mq);

   return *singleton;
}

Shm & Shm::instance() {

   if (!singleton)
      singleton = new Shm(-1, -1, INT_MAX, NULL);//34359738368); // 32GB

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

Object * Shm::getObjectFromHandle(const shm_handle_t & handle) {

   try {
      Object *object = static_cast<Object *>
         (shm->get_address_from_handle(handle));
      return object;

   } catch (boost::interprocess::interprocess_exception &ex) { }

   return NULL;
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

Triangles::Triangles(const size_t & corners, const size_t & vertices)
   : Object(Object::TRIANGLES), numCorners(corners), numVertices(vertices) {

   x = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   y = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   z = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));

   cl = static_cast<int *>
      (Shm::instance().getShm().allocate(numCorners * sizeof(int)));
}


Triangles * Triangles::create(const size_t & numCorners,
                              const size_t & numVertices) {

   std::string name = Shm::instance().createObjectID();
   Triangles *t = static_cast<Triangles *>
      (Shm::instance().getShm().construct<Triangles>(name.c_str())[1](numCorners, numVertices));

   shm_handle_t handle =
      Shm::instance().getShm().get_handle_from_address(t);

   Shm::instance().publish(handle);
   return t;
}

const size_t & Triangles::getNumCorners() const {

   return numCorners;
}

const size_t & Triangles::getNumVertices() const {

   return numVertices;
}

template<> const Object::Type Vec<float>::type  = Object::VECFLOAT;
template<> const Object::Type Vec<int>::type    = Object::VECINT;
template<> const Object::Type Vec<char>::type   = Object::VECCHAR;
template<> const Object::Type Vec3<float>::type = Object::VEC3FLOAT;
template<> const Object::Type Vec3<int>::type   = Object::VEC3INT;
template<> const Object::Type Vec3<char>::type  = Object::VEC3CHAR;

} // namespace vistle
