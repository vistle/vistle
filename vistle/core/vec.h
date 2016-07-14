#ifndef VEC_H
#define VEC_H

#include "index.h"
#include "dimensions.h"
#include "shm.h"
#include "object.h"
#include "vector.h"
#include "database.h"

namespace vistle {

template <typename T, int Dim=1>
class Vec: public DataBase {
   V_OBJECT(Vec);

   static const int MaxDim = MaxDimension;
   BOOST_STATIC_ASSERT(Dim > 0);
   BOOST_STATIC_ASSERT(Dim <= MaxDim);

 public:
   typedef DataBase Base;
   typedef typename shm<T>::array array;
   typedef T Scalar;
   typedef typename VistleScalarVector<Dim>::type Vector;

   Vec(const Index size,
        const Meta &meta=Meta());

   Index getSize() const override {
      return d()->x[0]->size();
   }

   void setSize(const Index size) override;

   array &x(int c=0) { return *d()->x[c]; }
   array &y() { return *d()->x[1]; }
   array &z() { return *d()->x[2]; }
   array &w() { return *d()->x[3]; }

   const T *x(int c=0) const { return m_x[c]; }
   const T *y() const { return m_x[1]; }
   const T *z() const { return m_x[2]; }
   const T *w() const { return m_x[3]; }

   std::pair<Vector, Vector> getMinMax() const;

 private:
   mutable const T *m_x[MaxDim];
   mutable Index m_size;

 public:
   struct Data: public Base::Data {

      ShmVector<T> x[Dim];
      // when used as Vec
      Data(const Index size = 0, const std::string &name = "",
            const Meta &meta=Meta());
      // when used as base of another data structure
      Data(const Index size, Type id, const std::string &name,
            const Meta &meta=Meta());
      Data(const Data &other, const std::string &name, Type id=UNKNOWN);
      Data(Object::Type id, const std::string &name, const Meta &meta);
      static Data *create(Index size=0, const Meta &meta=Meta());
      static Data *createNamed(Object::Type id, const std::string &name, const Meta &meta=Meta());

      private:
      friend class Vec;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

namespace boost {
template<typename S, int d>
struct is_virtual_base_of<typename vistle::Vec<S,d>::Base, vistle::Vec<S,d>>: public mpl::true_ {};
}

#ifdef VISTLE_IMPL
#include "vec_impl.h"
#endif
#endif
