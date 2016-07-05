#ifndef VEC_IMPL_H
#define VEC_IMPL_H

#include "scalars.h"
#include "archives.h"

#include <limits>

#include <boost/mpl/size.hpp>
#if 0
#include "celltree_impl.h"
#endif

namespace vistle {

template <class T, int Dim>
Vec<T,Dim>::Vec()
      : Base()
{
   refreshImpl();
}

template <class T, int Dim>
Vec<T,Dim>::Vec(Vec::Data *data)
      : Base(data)
{
   refreshImpl();
}

template <class T, int Dim>
Vec<T,Dim>::Vec(const Index size,
        const Meta &meta)
      : Base(Data::create(size, meta))
{
   refreshImpl();
}

template <class T, int Dim>
void Vec<T,Dim>::setSize(const Index size) {
   for (int c=0; c<Dim; ++c) {
      if (d()->x[c].valid()) {
         d()->x[c]->resize(size);
      } else {
         d()->x[c].construct(size);
      }
   }
   refresh();
}

template <class T, int Dim>
void Vec<T,Dim>::refreshImpl() const {

   const Data *d = static_cast<Data *>(m_data);

   for (int c=0; c<Dim; ++c) {
      m_x[c] = (d && d->x[c].valid()) ? d->x[c]->data() : nullptr;
   }
   for (int c=Dim; c<MaxDim; ++c) {
      m_x[c] = nullptr;
   }
   m_size = (d && d->x[0]) ? d->x[0]->size() : 0;
}

template <class T, int Dim>
bool Vec<T,Dim>::isEmpty() const {

   return getSize() == 0;
}

template <class T, int Dim>
bool Vec<T,Dim>::checkImpl() const {

   for (int c=0; c<Dim; ++c) {
      V_CHECK (d()->x[c]->check());
      V_CHECK (d()->x[c]->size() == d()->x[0]->size());
   }

   return true;
}

template <class T, int Dim>
std::pair<typename Vec<T,Dim>::Vector, typename Vec<T,Dim>::Vector> Vec<T,Dim>::getMinMax() const {

   Scalar smax = std::numeric_limits<Scalar>::max();
   Vector min, max;
   Index sz = getSize();
   for (int c=0; c<Dim; ++c) {
      min[c] = smax;
      max[c] = -smax;
      const Scalar *d = &x(c)[0];
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
Vec<T,Dim>::Data::Data(Object::Type id, const std::string &name, const Meta &m)
: Vec<T,Dim>::Base::Data(id, name, m)
{
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const Index size, const std::string &name,
      const Meta &m)
: Vec<T,Dim>::Base::Data(Vec<T,Dim>::type(), name, m)
{
   for (int c=0; c<Dim; ++c)
      x[c].construct(size);
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
       x[c].construct(size);
}

template <class T, int Dim>
Vec<T,Dim>::Data::Data(const Data &o, const std::string &n, Type id)
    : Vec<T,Dim>::Base::Data(o, n, id==Object::UNKNOWN ? o.type : id)
{
   for (int c=0; c<Dim; ++c)
      x[c] = o.x[c];
}

template <class T, int Dim>
typename Vec<T,Dim>::Data *Vec<T,Dim>::Data::create(Index size, const Meta &meta) {
   std::string name = Shm::the().createObjectId();
   Data *t = shm<Data>::construct(name)(size, name, meta);
   publish(t);

   return t;
}

template <class T, int Dim>
typename Vec<T,Dim>::Data *Vec<T,Dim>::Data::createNamed(Object::Type id, const std::string &name, const Meta &meta) {
   Data *t = shm<Data>::construct(name)(id, name, meta);
   publish(t);
   return t;
}

template <class T, int Dim>
template <class Archive>
void Vec<T,Dim>::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base_object", boost::serialization::base_object<Base::Data>(*this));
   int dim = Dim;
   ar & V_NAME("dim", dim);
   assert(dim == Dim);
   for (int c=0; c<Dim; ++c) {
      ar & V_NAME("x", x[c]);
   }
}

} // namespace vistle

#endif
