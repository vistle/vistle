#ifndef VEC_H
#define VEC_H

#include "scalar.h"
#include "dimensions.h"
#include "shm.h"
#include "object.h"


namespace vistle {

template <typename T, int Dim=1>
class V_COREEXPORT Vec: public Object {
   V_OBJECT(Vec);

   static const int MaxDim = 4;
   BOOST_STATIC_ASSERT(Dim > 0);
   BOOST_STATIC_ASSERT(Dim < MaxDim);

 public:
   typedef Object Base;

   typedef typename shm<T>::vector vector;

   Vec(const size_t size,
        const Meta &meta=Meta());

   size_t getSize() const {
      return d()->x[0]->size();
   }

   void setSize(const size_t size);

   static Type type();

   vector &x(int c=0) const { return *(*d()->x[c])(); }
   vector &y() const { return *(*d()->x[1])(); }
   vector &z() const { return *(*d()->x[2])(); }
   vector &w() const { return *(*d()->x[3])(); }

 protected:
   struct Data: public Base::Data {

      typename ShmVector<T>::ptr x[Dim];
      // when used as Vec
      Data(const size_t size = 0, const std::string &name = "",
            const Meta &meta=Meta());
      // when used as base of another data structure
      Data(const size_t size, Type id, const std::string &name,
            const Meta &meta=Meta());
      Data(const Data &other, const std::string &name);
      static Data *create(size_t size = 0, const Meta &meta=Meta());

      private:
      friend class Vec;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "vec_impl.h"
#endif
#endif
