#include <iostream>
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

FloatArray::FloatArray(const std::string &name): Object() {
   
   const FloatShmAllocator
      alloc_inst(Shm::instance().getShm().get_segment_manager());

   /*
   if (Shm::instance().getShm().find<FloatVector>(name.c_str()).first != NULL)
      std::cerr << "object [" << name << "] already exists" << std::endl;
   */
   vec = Shm::instance().getShm().construct<FloatVector>(name.c_str())(alloc_inst);
}
}
