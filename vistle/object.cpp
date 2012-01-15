#include <iostream>
#include <iomanip>
#include "object.h"

namespace vistle {

Shm* Shm::singleton = NULL;

Shm::Shm(const int m, const int r, const size_t &size)
   : moduleID(m), rank(r), objectID(0) {

   shm = new managed_shared_memory(open_or_create, "vistle", size);
}

Shm::~Shm() {

}

Shm & Shm::instance(const int moduleID, const int rank) {

   if (!singleton)
      singleton = new Shm(moduleID, rank, 2147483647);//34359738368); // 32GB

   return *singleton;
}

Shm & Shm::instance() {

   if (!singleton)
      singleton = new Shm(-1, -1, 2147483647);//34359738368); // 32GB

   return *singleton;
}

managed_shared_memory & Shm::getShm() {

   return *shm;
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
}

FloatArray::FloatArray(): Object() {

   const FloatShmAllocator
      alloc_inst(Shm::instance().getShm().get_segment_manager());

  vec = Shm::instance().getShm().construct<FloatVector>
     (Shm::instance().createObjectID().c_str())(alloc_inst);
}

} // namespace vistle
