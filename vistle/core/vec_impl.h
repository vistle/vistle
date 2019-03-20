#ifndef VEC_IMPL_H
#define VEC_IMPL_H

#include "scalars.h"
#include "structuredgridbase.h"

#include <limits>
#include <type_traits>

#include <boost/mpl/size.hpp>

namespace vistle {

template <class T, int Dim>
template <class Archive>
void Vec<T,Dim>::Data::serialize(Archive &ar) {

   ar & V_NAME(ar, "base_object", serialize_base<Base::Data>(ar, *this));
   int dim = Dim;
   ar & V_NAME(ar, "dim", dim);
   assert(dim == Dim);
   for (int c=0; c<Dim; ++c) {
      ar & V_NAME(ar, std::string("x" + std::to_string(c)).c_str(), x[c]);
   }
}

} // namespace vistle

#endif
