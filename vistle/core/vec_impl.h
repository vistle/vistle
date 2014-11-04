#ifndef VEC_IMPL_H
#define VEC_IMPL_H

#include "scalars.h"

#include <limits>

#include <boost/mpl/size.hpp>

namespace vistle {

template <class T, int Dim>
Vec<T,Dim>::Vec(const Index size,
        const Meta &meta)
      : Object(Data::create(size, meta)) {
   }

template <class T, int Dim>
void Vec<T,Dim>::setSize(const Index size) {
   for (int c=0; c<Dim; ++c)
      d()->x[c]->resize(size);
}

template <class T, int Dim>
bool Vec<T,Dim>::isEmpty() const {

   return getSize() == 0;
}

template <class T, int Dim>
bool Vec<T,Dim>::checkImpl() const {

   for (int c=1; c<Dim; ++c)
      V_CHECK (d()->x[c]->size() == d()->x[0]->size());

   return true;
}

template <class T, int Dim>
std::pair<typename Vec<T,Dim>::Vector, typename Vec<T,Dim>::Vector> Vec<T,Dim>::getMinMax() const {

   Scalar smax = std::numeric_limits<Scalar>::max();
   Vector min = Vector(smax, smax, smax), max = Vector(-smax, -smax, -smax);
   Index sz = getSize();
   for (int c=0; c<Dim; ++c) {
      const Scalar *d = x(c).data();
      for (Index i=0; i<sz; ++i) {
         if (d[i] < min[c])
            min[c] = d[i];
         if (d[i] > max[c])
            max[c] = d[i];

      }
   }
   return std::make_pair(min, max);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const Index size, const std::string &name,
      const Meta &m)
: Vec<T,Dim>::Base::Data(Vec<T,Dim>::type(), name, m)
{
   for (int c=0; c<Dim; ++c)
      x[c] = new ShmVector<T>(size);
}

template <class T, int Dim>
Object::Type Vec<T,Dim>::type() {

   static const size_t pos = boost::mpl::find<Scalars, T>::type::pos::value;
   BOOST_STATIC_ASSERT(pos < boost::mpl::size<Scalars>::value);
   return (Object::Type)(Object::VEC + (MaxDim+1)*pos + Dim);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const Index size, Type id, const std::string &name,
      const Meta &m)
: Vec<T,Dim>::Base::Data(id, name, m)
{
   for (int c=0; c<Dim; ++c)
      x[c] = new ShmVector<T>(size);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const Data &o, const std::string &n, Type id)
: Vec<T,Dim>::Base::Data(o, n, id)
{
   for (int c=0; c<Dim; ++c)
      x[c] = o.x[c];
}

template <class T, int Dim>
typename Vec<T,Dim>::Data *Vec<T,Dim>::Data::create(Index size, const Meta &meta) {
   std::string name = Shm::the().createObjectID();
   Data *t = shm<Data>::construct(name)(size, name, meta);
   publish(t);

   return t;
}

template <class T, int Dim>
template <class Archive>
void Vec<T,Dim>::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base:object", boost::serialization::base_object<Base::Data>(*this));
   int dim = Dim;
   ar & V_NAME("dim", dim);
   assert(dim == Dim);
   for (int c=0; c<Dim; ++c) {
      ar & V_NAME("x", *x[c]);
   }
}

} // namespace vistle

#endif
