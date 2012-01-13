#include "object.h"

namespace vistle {

Shm* Shm::singleton = NULL;

Shm::Shm(size_t size) {

   shm = new managed_shared_memory(open_or_create, "vistle", size);
}

Shm::~Shm() {
   
   //shared_memory_object::remove("vistle");
}

Shm & Shm::instance() {

   if (!singleton)
      singleton = new Shm(2147483647);//34359738368); // 32GB

   return *singleton;
}

managed_shared_memory & Shm::getShm() {
      
   return *shm;
}

Object::Object() {
   
}

Object::~Object() {
   
}

FloatArray::FloatArray(): Object() {
   
   const FloatShmAllocator
      alloc_inst(Shm::instance().getShm().get_segment_manager());

   vec = Shm::instance().getShm().construct<FloatVector>("FloatVector")(alloc_inst);
}
}
