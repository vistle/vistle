#ifndef OBJECT_H
#define OBJECT_H

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

// include headers that implement an archive in simple text format
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include "scalar.h"

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

namespace message {
   class MessageQueue;
}

class Object;

class Shm {

 public:
   static Shm & instance(const int moduleID = -1, const int rank = -1,
                         message::MessageQueue * messageQueue = NULL);
   ~Shm();

   boost::interprocess::managed_shared_memory & getShm();
   std::string createObjectID();

   void publish(const shm_handle_t & handle);
   Object * getObjectFromHandle(const shm_handle_t & handle);

 private:
   Shm(const int moduleID, const int rank, const size_t size,
       message::MessageQueue *messageQueue);

   const int m_moduleID;
   const int m_rank;
   int m_objectID;
   static Shm *s_singleton;
   boost::interprocess::managed_shared_memory *m_shm;
   message::MessageQueue *m_messageQueue;
};

template<typename T>
struct shm {
   typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> allocator;
   typedef boost::interprocess::vector<T, allocator> vector;
   typedef boost::interprocess::offset_ptr<vector> ptr;
   static allocator alloc_inst() { return allocator(Shm::instance().getShm().get_segment_manager()); }
   static ptr construct_vector(size_t s) { return Shm::instance().getShm().construct<vector>(Shm::instance().createObjectID().c_str())(s, T(), alloc_inst()); }
   static typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type construct(const std::string &name) { return Shm::instance().getShm().construct<T>(name.c_str()); }
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
      POLYGONS          =  8,
      UNSTRUCTUREDGRID  =  9,
      SET               = 10,
      GEOMETRY          = 11,
      TEXTURE1D         = 12,
   };

   struct Info {
      uint64_t infosize;
      uint64_t itemsize;
      uint64_t offset;
      uint64_t type;
      uint64_t block;
      uint64_t timestep;
      virtual ~Info() {}; // for RTTI

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & infosize;
            ar & itemsize;
            ar & offset;
            ar & type;
            ar & block;
            ar & timestep;
         }
   };

   Object(const Type id, const std::string & name,
          const int block, const int timestep);
   virtual ~Object();

   Info *getInfo(Info *info = NULL) const;

   Type getType() const;
   std::string getName() const;

   int getBlock() const;
   int getTimestep() const;

   void setBlock(const int block);
   void setTimestep(const int timestep);

 private:
   const Type m_type;
   char m_name[32];

   int m_block;
   int m_timestep;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         boost::scoped_ptr<Info> info(getInfo());
         ar & *info;
      }

};


template <class T>
class Vec: public Object {

 public:
   typedef Object Parent;

   struct Info: public Object::Info {
      uint64_t numElements;

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Parent::Info>(*this);
            ar & numElements;
         }
   };

   static Vec<T> * create(const size_t size = 0,
                          const int block = -1, const int timestep = -1) {

      const std::string name = Shm::instance().createObjectID();
      Vec<T> *t = shm<Vec<T> >::construct(name)(size, name, block, timestep);

      /*
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      */
      return t;
   }

 Vec(const size_t size, const std::string & name,
     const int block, const int timestep)
    : Object(s_type, name, block, timestep) {

       x = shm<T>::construct_vector(size);
   }

   Info *getInfo(Info *info = NULL) const;

   size_t getSize() const {
      return x->size();
   }

   void setSize(const size_t size) {
      x->resize(size);
   }

   typename shm<T>::ptr x;

 private:
   static const Object::Type s_type;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & x;
      }
};


template <class T>
class Vec3: public Object {

 public:
   typedef Object Parent;

   struct Info: public Object::Info {
      uint64_t numElements;
   };

   static Vec3<T> * create(const size_t size = 0,
                           const int block = -1, const int timestep = -1) {

      const std::string name = Shm::instance().createObjectID();
      Vec3<T> *t = shm<Vec3<T> >::construct(name)(size, name, block, timestep);
      /*
      shm_handle_t handle =
         Shm::instance().getShm().get_handle_from_address(t);
      Shm::instance().publish(handle);
      */
      return t;
   }

   Vec3(const size_t size, const std::string & name,
        const int block, const int timestep)
      : Object(s_type, name, block, timestep) {

      x = shm<T>::construct_vector(size);
      y = shm<T>::construct_vector(size);
      z = shm<T>::construct_vector(size);
   }

   Info *getInfo(Info *info = NULL) const;

   size_t getSize() const {
      return x->size();
   }

   void setSize(const size_t size) {
      x->resize(size);
      y->resize(size);
      z->resize(size);
   }

   typename shm<T>::ptr x, y, z;

 private:
   static const Object::Type s_type;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & x;
         ar & y;
         ar & z;
      }
};

class Triangles: public Object {

 public:
   typedef Object Parent;

   struct Info: public Object::Info {
      uint64_t numCorners;
      uint64_t numVertices;
   };

   static Triangles * create(const size_t numCorners = 0,
                             const size_t numVertices = 0,
                             const int block = -1, const int timestep = -1);

   Triangles(const size_t numCorners, const size_t numVertices,
             const std::string & name,
             const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::ptr cl;
   shm<Scalar>::ptr x, y, z;
 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & cl;
         ar & x;
         ar & y;
         ar & z;
      }
};

class Lines: public Object {

 public:
   typedef Object Parent;

   struct Info: public Object::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   static Lines * create(const size_t numElements = 0,
                         const size_t numCorners = 0,
                         const size_t numVertices = 0,
                         const int block = -1, const int timestep = -1);

   Lines(const size_t numElements, const size_t numCorners,
         const size_t numVertices, const std::string & name,
         const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::ptr el, cl;
   shm<Scalar>::ptr x, y, z;

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & el;
         ar & cl;
         ar & x;
         ar & y;
         ar & z;
      }
};

class Polygons: public Object {

 public:
   typedef Object Parent;

   struct Info: public Object::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   static Polygons * create(const size_t numElements = 0,
                            const size_t numCorners = 0,
                            const size_t numVertices = 0,
                            const int block = -1, const int timestep = -1);
   Polygons(const size_t numElements, const size_t numCorners,
            const size_t numVertices, const std::string & name,
            const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::ptr el, cl;
   shm<Scalar>::ptr x, y, z;

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & el;
         ar & cl;
         ar & x;
         ar & y;
         ar & z;
      }
};


class UnstructuredGrid: public Object {

 public:
   typedef Object Parent;

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

   struct Info: public Object::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   static UnstructuredGrid * create(const size_t numElements = 0,
                                    const size_t numCorners = 0,
                                    const size_t numVertices = 0,
                                    const int block = -1,
                                    const int timestep = -1);

   UnstructuredGrid(const size_t numElements, const size_t numCorners,
                    const size_t numVertices, const std::string & name,
                    const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<char>::ptr tl;
   shm<size_t>::ptr el, cl;
   shm<Scalar>::ptr x, y, z;

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & tl;
         ar & el;
         ar & cl;
         ar & x;
         ar & y;
         ar & z;
      }
};

class Set: public Object {

 public:
   typedef Object Parent;

   struct Info: public Object::Info {
      std::vector<Object::Info *> items;
   };

   static Set * create(const size_t numElements = 0,
                       const int block = -1, const int timestep = -1);

   Set(const size_t numElements, const std::string & name,
       const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   Object * getElement(const size_t index) const;

   shm<boost::interprocess::offset_ptr<Object> >::ptr elements;

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & elements;
      }
};

class Geometry: public Object {

 public:
   typedef Object Parent;

   static Geometry * create(const int block = -1, const int timestep = -1);

   Geometry(const std::string & name,
            const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;

   boost::interprocess::offset_ptr<Object> geometry;
   boost::interprocess::offset_ptr<Object> colors;
   boost::interprocess::offset_ptr<Object> normals;
   boost::interprocess::offset_ptr<Object> texture;

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & geometry;
         ar & colors;
         ar & normals;
         ar & texture;
      }
};

class Texture1D: public Object {

 public:
   typedef Object Parent;

   static Texture1D * create(const size_t width = 0,
                             const Scalar min = 0, const Scalar max = 0,
                             const int block = -1, const int timestep = -1);

   Texture1D(const std::string & name, const size_t size,
             const Scalar min, const Scalar max,
             const int block, const int timestep);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getWidth() const;

   Scalar min;
   Scalar max;

   shm<unsigned char>::ptr pixels;
   shm<Scalar>::ptr coords;

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & pixels;
         ar & coords;
         ar & min;
         ar & max;
      }
};

} // namespace vistle
#endif
