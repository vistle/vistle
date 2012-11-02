#ifndef OBJECTS_H
#define OBJECTS_H


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
#include "shm.h"
#include "object.h"

namespace vistle {

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
      // when used as Vec3
      Data(const size_t size, const std::string &name,
            const int block, const int timestep)
         : Base::Data(s_type, name, block, timestep)
            , x(new ShmVector<T>(size))
            , y(new ShmVector<T>(size))
            , z(new ShmVector<T>(size)) {
      }
      // when used as base of another data structure
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
