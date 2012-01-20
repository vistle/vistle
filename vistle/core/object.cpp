#include <iostream>
#include <iomanip>

#include <limits.h>

#include "message.h"
#include "messagequeue.h"
#include "object.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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

Object::Object(const Type type, const std::string & n): id(type) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;
}

Object::~Object() {

}

Object::Type Object::getType() const {

   return id;
}

std::string Object::getName() const {

   return name;
}

Triangles::Triangles(const size_t & corners, const size_t & vertices,
                     const std::string & name)
   : Object(Object::TRIANGLES, name),
     numCorners(corners), numVertices(vertices) {

   x = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   y = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   z = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));

   cl = static_cast<size_t *>
      (Shm::instance().getShm().allocate(numCorners * sizeof(size_t)));
}


Triangles * Triangles::create(const size_t & numCorners,
                              const size_t & numVertices) {

   const std::string name = Shm::instance().createObjectID();
   Triangles *t = static_cast<Triangles *>
      (Shm::instance().getShm().construct<Triangles>(name.c_str())[1](numCorners, numVertices, name));

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

Lines::Lines(const size_t & elements, const size_t & corners,
             const size_t & vertices, const std::string & name)
   : Object(Object::LINES, name),
     numElements(elements), numCorners(corners), numVertices(vertices) {

   x = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   y = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   z = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));

   el = static_cast<size_t *>
      (Shm::instance().getShm().allocate(numElements * sizeof(size_t)));

   cl = static_cast<size_t *>
      (Shm::instance().getShm().allocate(numCorners * sizeof(size_t)));
}


Lines * Lines::create(const size_t & numElements, const size_t & numCorners,
                      const size_t & numVertices) {

   const std::string name = Shm::instance().createObjectID();
   Lines *l = static_cast<Lines *>
      (Shm::instance().getShm().construct<Lines>(name.c_str())[1](numElements, numCorners, numVertices, name));

   shm_handle_t handle =
      Shm::instance().getShm().get_handle_from_address(l);

   Shm::instance().publish(handle);
   return l;
}

const size_t & Lines::getNumElements() const {

   return numElements;
}

const size_t & Lines::getNumCorners() const {

   return numCorners;
}

const size_t & Lines::getNumVertices() const {

   return numVertices;
}


UnstructuredGrid::UnstructuredGrid(const size_t & elements,
                                   const size_t & corners,
                                   const size_t & vertices,
                                   const std::string & name)
   : Object(Object::UNSTRUCTUREDGRID, name),
     numElements(elements), numCorners(corners), numVertices(vertices) {

   x = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   y = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));
   z = static_cast<float *>
      (Shm::instance().getShm().allocate(numVertices * sizeof(float)));

   tl = static_cast<Type *>
      (Shm::instance().getShm().allocate(numCorners * sizeof(Type)));

   el = static_cast<size_t *>
      (Shm::instance().getShm().allocate(numCorners * sizeof(size_t)));

   cl = static_cast<size_t *>
      (Shm::instance().getShm().allocate(numCorners * sizeof(size_t)));
}

UnstructuredGrid * UnstructuredGrid::create(const size_t & numElements,
                                            const size_t & numCorners,
                                            const size_t & numVertices) {

   const std::string name = Shm::instance().createObjectID();
   UnstructuredGrid *u = static_cast<UnstructuredGrid *>
      (Shm::instance().getShm().construct<UnstructuredGrid>(name.c_str())[1](numElements, numCorners, numVertices, name));

   shm_handle_t handle =
      Shm::instance().getShm().get_handle_from_address(u);

   Shm::instance().publish(handle);
   return u;
}

const size_t & UnstructuredGrid::getNumElements() const {

   return numElements;
}

const size_t & UnstructuredGrid::getNumCorners() const {

   return numCorners;
}

const size_t & UnstructuredGrid::getNumVertices() const {

   return numVertices;
}

template<> const Object::Type Vec<float>::type  = Object::VECFLOAT;
template<> const Object::Type Vec<int>::type    = Object::VECINT;
template<> const Object::Type Vec<char>::type   = Object::VECCHAR;
template<> const Object::Type Vec3<float>::type = Object::VEC3FLOAT;
template<> const Object::Type Vec3<int>::type   = Object::VEC3INT;
template<> const Object::Type Vec3<char>::type  = Object::VEC3CHAR;

} // namespace vistle
