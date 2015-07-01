#ifndef VEC_IMPL_H
#define VEC_IMPL_H

#include "scalars.h"

#include <limits>

#include <boost/mpl/size.hpp>
#include "celltree_impl.h"

namespace vistle {

template<class Archive>
void DataBase::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_object", boost::serialization::base_object<Base::Data>(*this));
}

template <class T, int Dim>
Vec<T,Dim>::Vec(const Index size,
        const Meta &meta)
      : Base(Data::create("", size, meta)) {
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
   Vector min, max;
   Index sz = getSize();
   for (int c=0; c<Dim; ++c) {
      min[c] = smax;
      max[c] = -smax;
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
typename Vec<T,Dim>::Data *Vec<T,Dim>::Data::create(const std::string &objId, Index size, const Meta &meta) {
   std::string name = Shm::the().createObjectId(objId);
   Data *t = shm<Data>::construct(name)(size, name, meta);
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

template <class T, int Dim>
void Vec<T,Dim>::createCelltree(Index nelem, const Index *el, const Index *cl) const {

   if (hasCelltree())
      return;

   const Scalar *coords[3] = {
      x().data(),
      y().data(),
      z().data()
   };
   const Scalar smax = std::numeric_limits<Scalar>::max();
   Vector vmin, vmax;
   vmin.fill(-smax);
   vmax.fill(smax);

   std::vector<Vector> min(nelem, vmax);
   std::vector<Vector> max(nelem, vmin);

   Vector gmin=vmax, gmax=vmin;
   for (Index i=0; i<nelem; ++i) {
      const Index start = el[i], end = el[i+1];
      for (Index c = start; c<end; ++c) {
         const Index v = cl[c];
         for (int d=0; d<3; ++d) {
            if (min[i][d] > coords[d][v]) {
               min[i][d] = coords[d][v];
               if (gmin[d] > min[i][d])
                  gmin[d] = min[i][d];
            }
            if (max[i][d] < coords[d][v]) {
               max[i][d] = coords[d][v];
               if (gmax[d] < max[i][d])
                  gmax[d] = max[i][d];
            }
         }
      }
   }

   typename Celltree::ptr ct(new Celltree(nelem));
   ct->init(min.data(), max.data(), gmin, gmax);
   addAttachment("celltree", ct);
}

} // namespace vistle

#endif
