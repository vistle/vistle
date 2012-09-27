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
   shm_handle_t getHandleFromObject(const Object *object);

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

#define V_OBJECT(Type) \
   public: \
   virtual ~Type() { delete d(); m_data = NULL; } \
   protected: \
   struct Data; \
   Data *d() const { return static_cast<Data *>(m_data); } \
   Type(Data *data) : Parent(data) {} \
   private: \
   friend Object *Shm::getObjectFromHandle(const shm_handle_t &); \
   friend shm_handle_t Shm::getHandleFromObject(const Object *); \
   friend Object *Object::create(Object::Data *)

class Object {
   friend Object *Shm::getObjectFromHandle(const shm_handle_t &);
   friend shm_handle_t Shm::getHandleFromObject(const Object *);

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

   virtual ~Object();

   Info *getInfo(Info *info = NULL) const;

   Type getType() const;
   std::string getName() const;

   int getBlock() const;
   int getTimestep() const;

   void setBlock(const int block);
   void setTimestep(const int timestep);

 protected:
   struct Data {
      const Type type;
      char name[32];

      int block;
      int timestep;

      Data(Type id, const std::string &name, int b, int t);
      static Data *create(Type id, int b, int t) {
         std::string name = Shm::instance().createObjectID();
         return shm<Data>::construct(name)(id, name, b, t);
      }

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
      }
   };

   Data *m_data;
   Data *d() const { return m_data; }

   Object(Data *data);

   static Object *create(Data *);
 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         boost::scoped_ptr<Info> info(getInfo());
         ar & *info;
      }

};

template <class T>
class Vec: public Object {
   V_OBJECT(Vec);

 public:
   typedef Object Parent;

   struct Info: public Parent::Info {
      uint64_t numElements;

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Parent::Info>(*this);
            ar & numElements;
         }
   };

   Vec(const size_t size = 0,
         const int block = -1, const int timestep = -1)
      : Object(Data::create(size, block, timestep)) {

   }

   Info *getInfo(Info *info = NULL) const;

   size_t getSize() const {
      return d()->x->size();
   }

   void setSize(const size_t size) {
      d()->x->resize(size);
   }

   typename shm<T>::vector &x() const { return *d()->x; }
   typename shm<T>::vector &x(int c) const { assert(c == 0 && "Vec only has one component"); return *d()->x; }

 protected:
   struct Data: public Parent::Data {

      typename shm<T>::ptr x;

      Data(size_t size, const std::string &name,
            const int block, const int timestep)
         : Parent::Data(s_type, name, block, timestep)
           , x(shm<T>::construct_vector(size)) {
           }
      static Data *create(size_t size, const int block, const int timestep) {
         std::string name = Shm::instance().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         /*
            shm_handle_t handle =
            Shm::instance().getShm().get_handle_from_address(t);
            Shm::instance().publish(handle);
            */
         return t;
      }

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & x;
      }
   };

 private:
   static const Object::Type s_type;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & d().x;
      }
};


template <class T>
class Vec3: public Object {
   V_OBJECT(Vec3);

 public:
   typedef Object Parent;

   struct Info: public Parent::Info {
      uint64_t numElements;
   };

   Vec3(const size_t size = 0,
        const int block = -1, const int timestep = -1)
      : Object(Data::create(size, block, timestep)) {

   }

   Info *getInfo(Info *info = NULL) const;

   size_t getSize() const {
      return d()->x->size();
   }

   void setSize(const size_t size) {
      d()->x->resize(size);
      d()->y->resize(size);
      d()->z->resize(size);
   }

   typename shm<T>::vector &x() const { return *d()->x; }
   typename shm<T>::vector &y() const { return *d()->y; }
   typename shm<T>::vector &z() const { return *d()->z; }
   typename shm<T>::vector &x(int c) const {
      assert(c >= 0 && c<=2 && "Vec3 only has three components");
      switch(c) {
         case 0:
         return *d()->x;
         case 1:
         return *d()->y;
         case 2:
         return *d()->z;
      }
   }

 protected:
   struct Data: public Parent::Data {

      typename shm<T>::ptr x, y, z;
      Data(const size_t size, const std::string &name,
            const int block, const int timestep)
         : Parent::Data(s_type, name, block, timestep)
            , x(shm<T>::construct_vector(size))
            , y(shm<T>::construct_vector(size))
            , z(shm<T>::construct_vector(size)) {
      }
      static Data *create(size_t size, const int block, const int timestep) {
         std::string name = Shm::instance().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         /*
            shm_handle_t handle =
            Shm::instance().getShm().get_handle_from_address(t);
            Shm::instance().publish(handle);
            */
         return t;
      }

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & x;
         ar & y;
         ar & z;
      }
   };

   //Data *d() const { return static_cast<Data *>(m_data); }

 private:
   static const Object::Type s_type;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & d()->x;
         ar & d()->y;
         ar & d()->z;
      }
};

class Triangles: public Object {
   V_OBJECT(Triangles);

 public:
   typedef Object Parent;

   struct Info: public Parent::Info {
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Triangles(const size_t numCorners = 0, const size_t numVertices = 0,
             const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &cl() const { return *d()->cl; }
   shm<Scalar>::vector &x() const { return *d()->x; }
   shm<Scalar>::vector &y() const { return *d()->y; }
   shm<Scalar>::vector &z() const { return *d()->z; }

 protected:
   struct Data: public Parent::Data {

      shm<size_t>::ptr cl;
      shm<Scalar>::ptr x, y, z;

      Data(const size_t numCorners, const size_t numVertices,
            const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numCorners, const size_t numVertices,
            const int block, const int timestep);
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & d()->cl;
         ar & d()->x;
         ar & d()->y;
         ar & d()->z;
      }
};

class Lines: public Object {
   V_OBJECT(Lines);

 public:
   typedef Object Parent;

   struct Info: public Parent::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Lines(const size_t numElements = 0, const size_t numCorners = 0,
         const size_t numVertices = 0,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &el() const { return *d()->el; }
   shm<size_t>::vector &cl() const { return *d()->cl; }
   shm<Scalar>::vector &x() const { return *d()->x; }
   shm<Scalar>::vector &y() const { return *d()->y; }
   shm<Scalar>::vector &z() const { return *d()->z; }

 protected:
   struct Data: public Parent::Data {

      shm<size_t>::ptr el, cl;
      shm<Scalar>::ptr x, y, z;

      Data(const size_t numElements, const size_t numCorners,
         const size_t numVertices, const std::string & name,
         const int block, const int timestep);
      static Data *create(const size_t numElements, const size_t numCorners,
         const size_t numVertices,
         const int block, const int timestep);

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Parent::Data>(*this);
            ar & el;
            ar & cl;
            ar & x;
            ar & y;
            ar & z;
         }
   };
};

class Polygons: public Object {
   V_OBJECT(Polygons);

 public:
   typedef Object Parent;

   struct Info: public Parent::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Polygons(const size_t numElements= 0,
            const size_t numCorners = 0,
            const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &el() const { return *d()->el; }
   shm<size_t>::vector &cl() const { return *d()->cl; }
   shm<Scalar>::vector &x() const { return *d()->x; }
   shm<Scalar>::vector &y() const { return *d()->y; }
   shm<Scalar>::vector &z() const { return *d()->z; }

 protected:
   struct Data: public Parent::Data {

      shm<size_t>::ptr el, cl;
      shm<Scalar>::ptr x, y, z;

      Data(const size_t numElements, const size_t numCorners,
            const size_t numVertices, const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numElements= 0,
            const size_t numCorners = 0,
            const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

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

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
      }
};


class UnstructuredGrid: public Object {
   V_OBJECT(UnstructuredGrid);

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

   struct Info: public Parent::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   UnstructuredGrid(const size_t numElements = 0,
         const size_t numCorners = 0,
         const size_t numVertices = 0,
         const int block = -1,
         const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<char>::vector &tl() const { return *d()->tl; }
   shm<size_t>::vector &el() const { return *d()->el; }
   shm<size_t>::vector &cl() const { return *d()->cl; }
   shm<Scalar>::vector &x() const { return *d()->x; }
   shm<Scalar>::vector &y() const { return *d()->y; }
   shm<Scalar>::vector &z() const { return *d()->z; }

 protected:
   struct Data: public Parent::Data {

      shm<char>::ptr tl;
      shm<size_t>::ptr el, cl;
      shm<Scalar>::ptr x, y, z;

      Data(const size_t numElements, const size_t numCorners,
                    const size_t numVertices, const std::string & name,
                    const int block, const int timestep);
      static Data *create(const size_t numElements = 0,
                                    const size_t numCorners = 0,
                                    const size_t numVertices = 0,
                                    const int block = -1,
                                    const int timestep = -1);

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

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
      }
};

class Set: public Object {
   V_OBJECT(Set);

 public:
   typedef Object Parent;

   struct Info: public Parent::Info {
      std::vector<Object::Info *> items;
   };

   Set(const size_t numElements = 0,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   Object * getElement(const size_t index) const;
   shm<boost::interprocess::offset_ptr<Object> >::vector &elements() const { return *d()->elements; }

 protected:
   struct Data: public Parent::Data {

      shm<boost::interprocess::offset_ptr<Object> >::ptr elements;

      Data(const size_t numElements, const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numElements = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
         ar & elements;
      }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
      }
};

class Geometry: public Object {
   V_OBJECT(Geometry);

 public:
   typedef Object Parent;

   Geometry(const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

   boost::interprocess::offset_ptr<Object> &geometry() const { return d()->geometry; }
   boost::interprocess::offset_ptr<Object> &colors() const { return d()->colors; }
   boost::interprocess::offset_ptr<Object> &normals() const { return d()->normals; }
   boost::interprocess::offset_ptr<Object> &texture() const { return d()->texture; }

 protected:
   struct Data: public Parent::Data {

      boost::interprocess::offset_ptr<Object> geometry;
      boost::interprocess::offset_ptr<Object> colors;
      boost::interprocess::offset_ptr<Object> normals;
      boost::interprocess::offset_ptr<Object> texture;

      Data(const std::string & name,
            const int block, const int timestep);
      static Data *create(const int block = -1, const int timestep = -1);

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

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
      }
};

class Texture1D: public Object {
   V_OBJECT(Texture1D);

 public:
   typedef Object Parent;

   Texture1D(const size_t width = 0,
         const Scalar min = 0, const Scalar max = 0,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getWidth() const;
   shm<unsigned char>::vector &pixels() const { return *d()->pixels; }
   shm<Scalar>::vector &coords() const { return *d()->coords; }

 protected:
   struct Data: public Parent::Data {
      Scalar min;
      Scalar max;

      shm<unsigned char>::ptr pixels;
      shm<Scalar>::ptr coords;

      static Data *create(const size_t width = 0,
            const Scalar min = 0, const Scalar max = 0,
            const int block = -1, const int timestep = -1);
      Data(const std::string &name, const size_t size,
            const Scalar min, const Scalar max,
            const int block, const int timestep);

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

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Object>(*this);
      }
};

} // namespace vistle
#endif
