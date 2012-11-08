#ifndef VEC_H
#define VEC_H


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
         std::string name = Shm::the().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         publish(t);

         return t;
      }

      private:
      friend class Vec;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
         ar & V_NAME("x", *x);
      }
   };

 private:
   static const Object::Type s_type;
};

} // namespace vistle
#endif
