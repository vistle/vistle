#ifndef OBJECT_H
#define OBJECT_H

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <vector>

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

/*
   typedef allocator<float, managed_shared_memory::segment_manager> FloatShmAllocator;
   typedef vector<float, FloatShmAllocator> FloatVector;
*/

namespace message {
   class MessageQueue;
}

class Object;

class Shm {

 public:
   static Shm & instance(const int moduleID, const int rank,
                         message::MessageQueue * messageQueue);
   static Shm & instance();
   ~Shm();

   boost::interprocess::managed_shared_memory & getShm();
   std::string createObjectID();

   void publish(const shm_handle_t & handle);
   Object *getObjectFromHandle(const shm_handle_t & handle);

 private:
   Shm(const int moduleID, const int rank, const size_t &size,
       message::MessageQueue *messageQueue);

   const int moduleID;
   const int rank;
   int objectID;
   static Shm *singleton;
   boost::interprocess::managed_shared_memory * shm;
   message::MessageQueue *messageQueue;
};

class Object {

 public:
   enum Type {
      UNKNOWN    = -1,
      VECFLOAT   = 0,
      VECINT     = 1,
      VECCHAR    = 2,
      VEC3FLOAT  = 3,
      VEC3INT    = 4,
      VEC3CHAR   = 5,
      TRIANGLES  = 6,
   };

   Object(const Type id);
   virtual ~Object();

   Type getType() const;

 protected:
   const Type id;
};


template <class T>
class Vec: public Object {

 public:
   static Vec<T> * create(const size_t & size) {

      std::string name = Shm::instance().createObjectID();
      Vec<T> *t = static_cast<Vec<T> *>
         (Shm::instance().getShm().construct<Vec<T> >(name.c_str())[1](size));
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      return t;
   }

   Vec(const size_t s)
      : Object(type), size(s) {

      x = static_cast<T*>(Shm::instance().getShm().allocate(size * sizeof(T)));
   }

   const size_t & getSize() const {
      return size;
   }

   boost::interprocess::offset_ptr<T> x;

 private:
   static const Object::Type type;
   const size_t size;
};


template <class T>
class Vec3: public Object {

 public:
   static Vec3<T> * create(const size_t & size) {

      std::string name = Shm::instance().createObjectID();
      Vec3<T> *t = static_cast<Vec3<T> *>
         (Shm::instance().getShm().construct<Vec3<T> >(name.c_str())[1](size));
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      return t;
   }

   Vec3(size_t s): Object(type), size(s) {

      x = static_cast<T*>(Shm::instance().getShm().allocate(size * sizeof(T)));
      y = static_cast<T*>(Shm::instance().getShm().allocate(size * sizeof(T)));
      z = static_cast<T*>(Shm::instance().getShm().allocate(size * sizeof(T)));
   }

   const size_t & getSize() const {
      return size;
   }

   boost::interprocess::offset_ptr<T> x;
   boost::interprocess::offset_ptr<T> y;
   boost::interprocess::offset_ptr<T> z;

 private:
   static const Object::Type type;
   const size_t size;
};

class Triangles: public Object {

 public:
   static Triangles * create(const size_t & size);
   Triangles(const size_t & size);

   const size_t & getSize();

   boost::interprocess::offset_ptr<float> vertices;

 private:
   const size_t size;
};

} // namespace vistle
#endif
