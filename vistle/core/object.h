#ifndef OBJECT_H
#define OBJECT_H


#include "tools.h"

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/shared_ptr.hpp>

// include headers that implement an archive in simple text format
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include "scalar.h"

#define SHMDEBUG

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;
typedef char shm_name_t[32];

namespace message {
   class MessageQueue;
}

class Object;

#ifdef SHMDEBUG
struct ShmDebugInfo {
   shm_name_t name;
   shm_handle_t handle;
   char deleted;
   char type;

   ShmDebugInfo(char type='\0', const std::string &name = "", shm_handle_t handle = 0)
      : handle(handle)
      , deleted(0)
      , type(type)
   {
      memset(this->name, '\0', sizeof(this->name));
      strncpy(this->name, name.c_str(), sizeof(this->name)-1);
   }
};

template<typename T>
class ShmVector;
#endif

template<typename T>
struct shm {
   typedef boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> allocator;
   typedef boost::interprocess::vector<T, allocator> vector;
   typedef boost::interprocess::offset_ptr<vector> ptr;
   static allocator alloc_inst();
   //static ptr construct_vector(size_t s) { return Shm::instance().getShm().construct<vector>(Shm::instance().createObjectID().c_str())(s, T(), alloc_inst()); }
   static typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type construct(const std::string &name);
   static void destroy(const std::string &name);
};

class Shm {

 public:
   static Shm & instance();
   static Shm & create(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   static Shm & attach(const std::string &shmname, const int moduleID, const int rank,
                         message::MessageQueue *messageQueue = NULL);
   ~Shm();

   const std::string &getName() const;

   boost::interprocess::managed_shared_memory & getShm();
   std::string createObjectID();

   void publish(const shm_handle_t & handle);
   boost::shared_ptr<const Object> getObjectFromHandle(const shm_handle_t & handle);
   shm_handle_t getHandleFromObject(boost::shared_ptr<const Object> object);

   static std::string shmIdFilename();
   static bool cleanAll();

#ifdef SHMDEBUG
   static shm<ShmDebugInfo>::vector *s_shmdebug;
   void markAsRemoved(const std::string &name);
#endif

 private:
   Shm(const std::string &name, const int moduleID, const int rank, const size_t size,
       message::MessageQueue *messageQueue, bool create);

   std::string m_name;
   bool m_created;
   const int m_moduleID;
   const int m_rank;
   int m_objectID;
   static Shm *s_singleton;
   boost::interprocess::managed_shared_memory *m_shm;
   message::MessageQueue *m_messageQueue;
};

template<typename T>
typename shm<T>::allocator shm<T>::alloc_inst() {
   return allocator(Shm::instance().getShm().get_segment_manager());
}

//static ptr construct_vector(size_t s) { return Shm::instance().getShm().construct<vector>(Shm::instance().createObjectID().c_str())(s, T(), alloc_inst()); }

template<typename T>
typename boost::interprocess::managed_shared_memory::segment_manager::template construct_proxy<T>::type shm<T>::construct(const std::string &name) {
   return Shm::instance().getShm().construct<T>(name.c_str());
}

template<typename T>
void shm<T>::destroy(const std::string &name) {
      Shm::instance().getShm().destroy<T>(name.c_str());
#ifdef SHMDEBUG
      Shm::instance().markAsRemoved(name);
#endif
}

template<typename T>
class ShmVector {
#ifdef SHMDEBUG
   friend void Shm::markAsRemoved(const std::string &name);
#endif

   public:
      class ptr {
         public:
            ptr(ShmVector *p) : m_p(p) {
               m_p->ref();
            }
            ~ptr() {
               m_p->unref();
            }
            ShmVector &operator*() {
               return *m_p;
            }
            ShmVector *operator->() {
               return &*m_p;
            }
            ptr &operator=(ptr &other) {
               m_p = other.m_p;
               m_p->ref();
            }

         private:
            boost::interprocess::offset_ptr<ShmVector> m_p;
      };

      ShmVector(size_t size = 0)
         : m_refcount(0)
      {
         std::string n(Shm::instance().createObjectID());
         size_t nsize = n.size();
         if (nsize >= sizeof(m_name)) {
            nsize = sizeof(m_name)-1;
         }
         n.copy(m_name, nsize);
         assert(n.size() < sizeof(m_name));
         m_x = Shm::instance().getShm().construct<typename shm<T>::vector>(m_name)(size, T(), shm<T>::alloc_inst());
#ifdef SHMDEBUG
         shm_handle_t handle = Shm::instance().getShm().get_handle_from_address(this);
         Shm::instance().s_shmdebug->push_back(ShmDebugInfo('V', m_name, handle));
#endif
      }
      ~ShmVector() {
         shm<T>::destroy(m_name);
      }
      void ref() {
         boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_mutex);
         ++m_refcount;
      }
      void unref() {
         boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_mutex);
         --m_refcount;
         if (m_refcount == 0) {
            delete this;
         }
      }
      int refcount() const {
         return m_refcount;
      }
      void* operator new(size_t size) {
         return Shm::instance().getShm().allocate(size);
      }
      void operator delete(void *p) {
         return Shm::instance().getShm().deallocate(p);
      }

      T &operator[](size_t i) { return (*m_x)[i]; }
      const T &operator[](size_t i) const { return (*m_x)[i]; }

      size_t size() const { return m_x->size(); }
      void resize(size_t s) { m_x->resize(s); }

      typename shm<T>::ptr &operator()() { return m_x; }
      typename shm<const T>::ptr &operator()() const { return m_x; }

      void push_back(const T &d) { m_x->push_back(d); }

   private:
      boost::interprocess::interprocess_mutex m_mutex;
      int m_refcount;
      shm_name_t m_name;
      typename shm<T>::ptr m_x;
};

#define V_OBJECT(Type) \
   public: \
   typedef boost::shared_ptr<Type> ptr; \
   typedef boost::shared_ptr<const Type> const_ptr; \
   static boost::shared_ptr<const Type> as(boost::shared_ptr<const Object> ptr) { return boost::dynamic_pointer_cast<const Type>(ptr); } \
   static boost::shared_ptr<Type> as(boost::shared_ptr<Object> ptr) { return boost::dynamic_pointer_cast<Type>(ptr); } \
   virtual ~Type() { if (m_data) { d()->unref(); if (d()->refcount == 0) { shm<Type::Data>::destroy(getName()); } m_data = NULL; } } \
   protected: \
   struct Data; \
   Data *d() const { return static_cast<Data *>(m_data); } \
   Type(Data *data) : Base(data) {} \
   private: \
   friend boost::shared_ptr<const Object> Shm::getObjectFromHandle(const shm_handle_t &); \
   friend shm_handle_t Shm::getHandleFromObject(boost::shared_ptr<const Object>); \
   friend boost::shared_ptr<Object> Object::create(Object::Data *)

class Object {
   friend boost::shared_ptr<const Object> Shm::getObjectFromHandle(const shm_handle_t &);
   friend shm_handle_t Shm::getHandleFromObject(boost::shared_ptr<const Object>);
#ifdef SHMDEBUG
   friend void Shm::markAsRemoved(const std::string &name);
#endif

public:
   typedef boost::shared_ptr<Object> ptr;
   typedef boost::shared_ptr<const Object> const_ptr;

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

   void ref() const {
      d()->ref();
   }

   void unref() const {
      d()->unref();
   }

   int refcount() const {
      return d()->refcount;
   }

 protected:
   struct Data {
      const Type type;
      shm_name_t name;
      int refcount;
      boost::interprocess::interprocess_mutex mutex;

      int block;
      int timestep;

      Data(Type id, const std::string &name, int b, int t);
      void ref() {
         boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mutex);
         ++refcount;
      }
      void unref() {
         boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mutex);
         --refcount;
#if 0
         if (refcount == 0) {
            shm<Data>::destroy(name);
         }
#endif
      }
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
 public:
   Data *d() const { return m_data; }
 protected:

   Object(Data *data);

   static void publish(const Data *d);
   static Object::ptr create(Data *);
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
   typedef Object Base;

   struct Info: public Base::Info {
      uint64_t numElements;

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Base::Info>(*this);
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

   typename shm<T>::vector &x() const { return *(*d()->x)(); }
   typename shm<T>::vector &x(int c) const { assert(c == 0 && "Vec only has one component"); return x(); }

 protected:
   struct Data: public Base::Data {

      typename ShmVector<T>::ptr x;

      Data(size_t size, const std::string &name,
            const int block, const int timestep)
         : Base::Data(s_type, name, block, timestep)
           , x(new ShmVector<T>(size)) {
           }
      static Data *create(size_t size, const int block, const int timestep) {
         std::string name = Shm::instance().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         publish(t);

         return t;
      }

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & boost::serialization::base_object<Base::Data>(*this);
         ar & x;
      }
   };

 private:
   static const Object::Type s_type;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
         ar & d().x;
      }
};


template <class T>
class Vec3: public Object {
   V_OBJECT(Vec3);

 public:
   typedef Object Base;

   struct Info: public Base::Info {
      uint64_t numElements;
   };

   Vec3(const size_t size = 0,
        const int block = -1, const int timestep = -1)
      : Object(Data::create(size, block, timestep)) {

   }

   Info *getInfo(Info *info = NULL) const {

      if (!info) {
         info = new Info;
      }
      Base::getInfo(info);

      info->infosize = sizeof(Info);
      info->itemsize = 0;
      info->offset = 0;
      info->numElements = getSize();

      return info;
   }

   size_t getSize() const {
      return d()->x->size();
   }

   void setSize(const size_t size) {
      d()->x->resize(size);
      d()->y->resize(size);
      d()->z->resize(size);
   }

   typename shm<T>::vector &x() const { return *(*d()->x)(); }
   typename shm<T>::vector &y() const { return *(*d()->y)(); }
   typename shm<T>::vector &z() const { return *(*d()->z)(); }
   typename shm<T>::vector &x(int c) const {
      assert(c >= 0 && c<=2 && "Vec3 only has three components");
      switch(c) {
         case 0:
         return x();
         case 1:
         return y();
         case 2:
         return z();
      }
   }

 protected:
   struct Data: public Base::Data {

      typename ShmVector<T>::ptr x, y, z;
      Data(const size_t size, const std::string &name,
            const int block, const int timestep)
         : Base::Data(s_type, name, block, timestep)
            , x(new ShmVector<T>(size))
            , y(new ShmVector<T>(size))
            , z(new ShmVector<T>(size)) {
      }
      Data(const size_t size, Type id, const std::string &name,
            const int block, const int timestep)
         : Base::Data(id, name, block, timestep)
            , x(new ShmVector<T>(size))
            , y(new ShmVector<T>(size))
            , z(new ShmVector<T>(size)) {
      }
      static Data *create(size_t size, const int block, const int timestep) {
         std::string name = Shm::instance().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         publish(t);

         return t;
      }

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & boost::serialization::base_object<Base::Data>(*this);
         ar & x;
         ar & y;
         ar & z;
      }
   };

 private:
   static const Object::Type s_type;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
         ar & d()->x;
         ar & d()->y;
         ar & d()->z;
      }
};


class Coords: public Vec3<Scalar> {
   V_OBJECT(Coords);

 public:
   typedef Vec3<Scalar> Base;

   struct Info: public Base::Info {
      uint64_t numVertices;
   };

   Coords(const size_t numVertices = 0,
             const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumCoords() const;
   size_t getNumVertices() const;

 protected:
   struct Data: public Base::Data {

      Data(const size_t numVertices,
            Type id, const std::string & name,
            const int block, const int timestep);
      static Data *create(Type id, const size_t numVertices,
            const int block, const int timestep);

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Base::Data>(*this);
         }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};

class Points: public Coords {
   V_OBJECT(Points);

   public:
   typedef Coords Base;

   size_t getNumPoints() const;

   protected:
   struct Data: public Base::Data {

      Data(const size_t numPoints,
            const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numPoints,
            const int block, const int timestep);

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Base::Data>(*this);
         }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};

class Triangles: public Coords {
   V_OBJECT(Triangles);

 public:
   typedef Coords Base;

   struct Info: public Base::Info {
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Triangles(const size_t numCorners = 0, const size_t numVertices = 0,
             const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &cl() const { return *(*d()->cl)(); }

 protected:
   struct Data: public Base::Data {

      ShmVector<size_t>::ptr cl;

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
         ar & boost::serialization::base_object<Base>(*this);
         ar & d()->cl;
      }
};

class Indexed: public Coords {
   V_OBJECT(Indexed);

 public:
   typedef Coords Base;

   struct Info: public Base::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &cl() const { return *(*d()->cl)(); }
   shm<size_t>::vector &el() const { return *(*d()->el)(); }

 protected:

   struct Data: public Base::Data {
      ShmVector<size_t>::ptr el, cl;

      Data(const size_t numElements, const size_t numCorners,
           const size_t numVertices,
            Type id, const std::string &name,
            int b = -1, int t = -1);
   };
};

class Lines: public Indexed {
   V_OBJECT(Lines);

 public:
   typedef Indexed Base;

   struct Info: public Base::Info {
   };

   Info *getInfo(Info *info = NULL) const;

   Lines(const size_t numElements = 0, const size_t numCorners = 0,
         const size_t numVertices = 0,
         const int block = -1, const int timestep = -1);

 protected:
   struct Data: public Base::Data {

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
            ar & boost::serialization::base_object<Base::Data>(*this);
         }
   };
};

class Polygons: public Indexed {
   V_OBJECT(Polygons);

 public:
   typedef Indexed Base;

   struct Info: public Base::Info {
   };

   Polygons(const size_t numElements= 0,
            const size_t numCorners = 0,
            const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

 protected:
   struct Data: public Base::Data {

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
         ar & boost::serialization::base_object<Base>(*this);
      }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};


class UnstructuredGrid: public Indexed {
   V_OBJECT(UnstructuredGrid);

 public:
   typedef Indexed Base;

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

   struct Info: public Base::Info {
   };

   UnstructuredGrid(const size_t numElements = 0,
         const size_t numCorners = 0,
         const size_t numVertices = 0,
         const int block = -1,
         const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

   shm<char>::vector &tl() const { return *(*d()->tl)(); }

 protected:
   struct Data: public Base::Data {

      ShmVector<char>::ptr tl;

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
         ar & boost::serialization::base_object<Base::Data>(*this);
         ar & tl;
      }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};

class Set: public Object {
   V_OBJECT(Set);

 public:
   typedef Object Base;

   struct Info: public Base::Info {
      std::vector<Object::Info *> items;
   };

   Set(const size_t numElements = 0,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   Object::const_ptr getElement(const size_t index) const;
   void setElement(const size_t index, Object::const_ptr obj);
   void addElement(Object::const_ptr obj);
   shm<boost::interprocess::offset_ptr<Object::Data> >::vector &elements() const { return *(*d()->elements)(); }

 protected:
   struct Data: public Base::Data {

      ShmVector<boost::interprocess::offset_ptr<Object::Data> >::ptr elements;

      Data(const size_t numElements, const std::string & name,
            const int block, const int timestep);
      ~Data();
      static Data *create(const size_t numElements = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base::Data>(*this);
         ar & elements;
      }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};

class Geometry: public Object {
   V_OBJECT(Geometry);

 public:
   typedef Object Base;

   Geometry(const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

   void setGeometry(Object::const_ptr g);
   void setColors(Object::const_ptr c);
   void setTexture(Object::const_ptr t);
   void setNormals(Object::const_ptr n);
   Object::const_ptr geometry() const;
   Object::const_ptr colors() const;
   Object::const_ptr normals() const;
   Object::const_ptr texture() const;

 protected:
   struct Data: public Base::Data {

      boost::interprocess::offset_ptr<Object::Data> geometry;
      boost::interprocess::offset_ptr<Object::Data> colors;
      boost::interprocess::offset_ptr<Object::Data> normals;
      boost::interprocess::offset_ptr<Object::Data> texture;

      Data(const std::string & name,
            const int block, const int timestep);
      ~Data();
      static Data *create(const int block = -1, const int timestep = -1);

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base::Data>(*this);
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
         ar & boost::serialization::base_object<Base>(*this);
      }
};

class Texture1D: public Object {
   V_OBJECT(Texture1D);

 public:
   typedef Object Base;

   Texture1D(const size_t width = 0,
         const Scalar min = 0, const Scalar max = 0,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getWidth() const;
   shm<unsigned char>::vector &pixels() const { return *(*d()->pixels)(); }
   shm<Scalar>::vector &coords() const { return *(*d()->coords)(); }

 protected:
   struct Data: public Base::Data {
      Scalar min;
      Scalar max;

      ShmVector<unsigned char>::ptr pixels;
      ShmVector<Scalar>::ptr coords;

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
         ar & boost::serialization::base_object<Base::Data>(*this);
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
         ar & boost::serialization::base_object<Base>(*this);
      }
};

} // namespace vistle
#endif
