#ifndef VEC_IMPL_H
#define VEC_IMPL_H

#include "scalars.h"

namespace vistle {

template <class T, int Dim>
Vec<T,Dim>::Vec(const size_t size,
        const int block, const int timestep)
      : Object(Data::create(size, block, timestep)) {
   }

template <class T, int Dim>
void Vec<T,Dim>::setSize(const size_t size) {
   for (int c=0; c<Dim; ++c)
      d()->x[c]->resize(size);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const size_t size, const std::string &name,
      const int block, const int timestep)
: Vec<T,Dim>::Base::Data(Vec<T,Dim>::type(), name, block, timestep)
{
   for (int c=0; c<Dim; ++c)
      x[c] = new ShmVector<T>(size);
}

template <class T, int Dim>
Object::Type Vec<T,Dim>::type() {

   return (Object::Type)(Object::VEC + 5*boost::mpl::find<Scalars, T>::type::pos::value + Dim);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const size_t size, Type id, const std::string &name,
      const int block, const int timestep)
: Vec<T,Dim>::Base::Data(id, name, block, timestep)
{
   for (int c=0; c<Dim; ++c)
      x[c] = new ShmVector<T>(size);
}

template <class T, int Dim>
typename Vec<T,Dim>::Data *Vec<T,Dim>::Data::create(size_t size, const int block, const int timestep) {
   std::string name = Shm::the().createObjectID();
   Data *t = shm<Data>::construct(name)(size, name, block, timestep);
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
