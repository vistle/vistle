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

   Vec(const size_t size,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

   size_t getSize() const {
      return d()->x->size();
   }

   void setSize(const size_t size);

   typename shm<T>::vector &x() const { return *(*d()->x)(); }
   typename shm<T>::vector &x(int c) const { assert(c == 0 && "Vec only has one component"); return x(); }

 protected:
   struct Data: public Base::Data {

      typename ShmVector<T>::ptr x;

      Data(size_t size = 0, const std::string &name = "",
            const int block = -1, const int timestep = -1)
         : Base::Data(s_type, name, block, timestep)
           , x(new ShmVector<T>(size)) {
           }
      static Data *create(size_t size = 0, const int block = -1, const int timestep = -1) {
         std::string name = Shm::the().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         publish(t);

         return t;
      }

      private:
      friend class Vec;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
   };

 private:
   static const Object::Type s_type;
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "vec_impl.h"
#endif
#endif
