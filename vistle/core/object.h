#ifndef OBJECT_H
#define OBJECT_H

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <vector>

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

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
   Object * getObjectFromHandle(const shm_handle_t & handle);

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
      UNKNOWN           = -1,
      VECFLOAT          =  0,
      VECINT            =  1,
      VECCHAR           =  2,
      VEC3FLOAT         =  3,
      VEC3INT           =  4,
      VEC3CHAR          =  5,
      TRIANGLES         =  6,
      LINES             =  7,
      UNSTRUCTUREDGRID  =  8,
   };

   Object(const Type id, const std::string & name);
   virtual ~Object();

   Type getType() const;
   std::string getName() const;

 protected:
   const Type id;
 private:
   char name[32];
};


template <class T>
class Vec: public Object {

 public:
   static Vec<T> * create(const size_t & size = 0) {

      const std::string name = Shm::instance().createObjectID();
      Vec<T> *t = static_cast<Vec<T> *>
         (Shm::instance().getShm().construct<Vec<T> >(name.c_str())[1](size, name));
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      return t;
   }

 Vec(const size_t & size, const std::string & name)
      : Object(type, name) {

      const boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>
         alloc_inst(Shm::instance().getShm().get_segment_manager());

      x = Shm::instance().getShm().construct<std::vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> > > (Shm::instance().createObjectID().c_str())(size, float(), alloc_inst);
   }

   size_t getSize() const {
      return x->size();
   }

   void setSize(const size_t & size) {
      x->resize(size);
   }

   boost::interprocess::offset_ptr<std::vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> > >
      x;

 private:
   static const Object::Type type;
};


template <class T>
class Vec3: public Object {

 public:
   static Vec3<T> * create(const size_t & size = 0) {

      const std::string name = Shm::instance().createObjectID();
      Vec3<T> *t = static_cast<Vec3<T> *>
         (Shm::instance().getShm().construct<Vec3<T> >(name.c_str())[1](size, name));
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      return t;
   }

   Vec3(const size_t & size, const std::string & name)
      : Object(type, name) {

      const boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>
         alloc_inst(Shm::instance().getShm().get_segment_manager());

      x = Shm::instance().getShm().construct<std::vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> > > (Shm::instance().createObjectID().c_str())(size, float(), alloc_inst);
      y = Shm::instance().getShm().construct<std::vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> > > (Shm::instance().createObjectID().c_str())(size, float(), alloc_inst);
      z = Shm::instance().getShm().construct<std::vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> > > (Shm::instance().createObjectID().c_str())(size, float(), alloc_inst);
   }

   size_t getSize() const {
      return x->size();
   }

   void setSize(const size_t & size) {
      x->resize(size);
      y->resize(size);
      z->resize(size);
   }

   boost::interprocess::offset_ptr<std::vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> > >
      x, y, z;

 private:
   static const Object::Type type;
};

class Triangles: public Object {

 public:
   static Triangles * create(const size_t & numCorners = 0,
                             const size_t & numVertices = 0);

   Triangles(const size_t & numCorners, const size_t & numVertices,
             const std::string & name);

   size_t getNumCorners() const;
   size_t getNumVertices() const;

   boost::interprocess::offset_ptr<std::vector<size_t, boost::interprocess::allocator<size_t, boost::interprocess::managed_shared_memory::segment_manager> > >
      cl;

   boost::interprocess::offset_ptr<std::vector<float, boost::interprocess::allocator<float, boost::interprocess::managed_shared_memory::segment_manager> > >
      x, y, z;
 private:
};

class Lines: public Object {

 public:
   static Lines * create(const size_t & numElements = 0,
                         const size_t & numCorners = 0,
                         const size_t & numVertices = 0);
   Lines(const size_t & numElements, const size_t & numCorners,
         const size_t & numVertices, const std::string & name);

   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   boost::interprocess::offset_ptr<std::vector<size_t, boost::interprocess::allocator<size_t, boost::interprocess::managed_shared_memory::segment_manager> > >
      el, cl;
   boost::interprocess::offset_ptr<std::vector<float, boost::interprocess::allocator<float, boost::interprocess::managed_shared_memory::segment_manager> > >
      x, y, z;

 private:
};


class UnstructuredGrid: public Object {

 public:
   enum Type {
      NONE        =  0,
      BAR         =  1,
      TRIANGLE    =  2,
      QUAD        =  3,
      TETRAHEDRON =  4,
      PYRAMID     =  5,
      PRISM       =  6,
      HEXAHEDRON  =  7,
      POINT       = 10,
      POLYHEDRON  = 11
   };

   static UnstructuredGrid * create(const size_t & numElements = 0,
                                    const size_t & numCorners = 0,
                                    const size_t & numVertices = 0);

   UnstructuredGrid(const size_t & numElements, const size_t & numCorners,
                    const size_t & numVertices, const std::string & name);

   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   boost::interprocess::offset_ptr<std::vector<size_t, boost::interprocess::allocator<size_t, boost::interprocess::managed_shared_memory::segment_manager> > >
      tl, cl, el;

   boost::interprocess::offset_ptr<std::vector<float, boost::interprocess::allocator<float, boost::interprocess::managed_shared_memory::segment_manager> > >
      x, y, z;

 private:

};

} // namespace vistle
#endif
