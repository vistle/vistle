#ifndef VEC3_H
#define VEC3_H

#include "scalar.h"
#include "shm.h"
#include "object.h"

namespace vistle {

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
         std::string name = Shm::the().createObjectID();
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
};

} // namespace vistle
#endif
