#ifndef OBJECT_H
#define OBJECT_H

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <vector>

using namespace boost::interprocess;

namespace vistle {
   typedef allocator<float, managed_shared_memory::segment_manager> FloatShmAllocator;
   typedef vector<float, FloatShmAllocator> FloatVector;

class Shm {

 public:
   static Shm & instance();
   ~Shm();

   managed_shared_memory & getShm();

 private:
   Shm(size_t size);

   static Shm *singleton;
   managed_shared_memory *shm;
};

class Object {

 public:
   Object();
   ~Object();
};


class FloatArray: public Object {

 public:
   FloatArray(const std::string & name);
   FloatVector *vec;
};

}

#endif
