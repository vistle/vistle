#ifndef VEC_IMPL_H
#define VEC_IMPL_H

#include "scalars.h"

namespace vistle {

template <class T, int Dim>
Vec<T,Dim>::Vec(const size_t size,
        const Meta &meta)
      : Object(Data::create(size, meta)) {
   }

template <class T, int Dim>
void Vec<T,Dim>::setSize(const size_t size) {
   for (int c=0; c<Dim; ++c)
      d()->x[c]->resize(size);
}

template <class T, int Dim>
bool Vec<T,Dim>::checkImpl() const {

   for (int c=1; c<Dim; ++c)
      V_CHECK (d()->x[c]->size() == d()->x[0]->size());

   return true;
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const size_t size, const std::string &name,
      const Meta &m)
: Vec<T,Dim>::Base::Data(Vec<T,Dim>::type(), name, m)
{
   for (int c=0; c<Dim; ++c)
      x[c] = new ShmVector<T>(size);
}

template <class T, int Dim>
Object::Type Vec<T,Dim>::type() {

   return (Object::Type)(Object::VEC + (MaxDim+1)*boost::mpl::find<Scalars, T>::type::pos::value + Dim);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const size_t size, Type id, const std::string &name,
      const Meta &m)
: Vec<T,Dim>::Base::Data(id, name, m)
{
   for (int c=0; c<Dim; ++c)
      x[c] = new ShmVector<T>(size);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const Data &o, const std::string &n)
: Vec<T,Dim>::Base::Data(o, n)
{
   for (int c=0; c<Dim; ++c)
      x[c] = o.x[c];
}

template <class T, int Dim>
typename Vec<T,Dim>::Data *Vec<T,Dim>::Data::create(size_t size, const Meta &meta) {
   std::string name = Shm::the().createObjectID();
   Data *t = shm<Data>::construct(name)(size, name, meta);
   publish(t);

   return t;
}

template <class T, int Dim>
template <class Archive>
void Vec<T,Dim>::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
   int dim = Dim;
   ar & V_NAME("dim", dim);
   assert(dim == Dim);
   for (int c=0; c<Dim; ++c)
      ar & V_NAME("x", *x[c]);
}

} // namespace vistle

#endif
