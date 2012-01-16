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

namespace message {
   class MessageQueue;
}

class Shm {

 public:
   static Shm & instance(const int moduleID, const int rank,
                         message::MessageQueue *messageQueue);
   static Shm & instance();
   ~Shm();

   managed_shared_memory & getShm();
   std::string createObjectID();

   void publish(const std::string &name);

 private:
   Shm(const int moduleID, const int rank, const size_t &size,
       message::MessageQueue *messageQueue);

   const int moduleID;
   const int rank;
   int objectID;
   static Shm *singleton;
   managed_shared_memory *shm;
   message::MessageQueue *messageQueue;
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
