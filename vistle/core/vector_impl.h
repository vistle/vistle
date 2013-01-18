#ifndef VECTOR_IMPL_H
#define VECTOR_IMPL_H

#include <sstream>
#include <cassert>
#include <boost/static_assert.hpp>

namespace vistle {

BOOST_STATIC_ASSERT(MaxDimension >= 4);
template<typename S>
GenericVector<S>::GenericVector(const int dim, const S values[])
: dim(dim)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   assert(dim <= MaxDimension);
   for (int i=0; i<dim; ++i)
      v[i] = values[i];
   for (int i=dim; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
GenericVector<S>::GenericVector(const S _x, const S _y, const S _z, const S _w)
: dim(4)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   v[0] = _x;
   v[1] = _y;
   v[2] = _z;
   v[3] = _w;
   for (int i=4; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
GenericVector<S>::GenericVector(const S _x, const S _y, const S _z)
: dim(3)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   v[0] = _x;
   v[1] = _y;
   v[2] = _z;
   for (int i=3; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
GenericVector<S>::GenericVector(const S _x, const S _y)
: dim(2)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   v[0] = _x;
   v[1] = _y;
   for (int i=2; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
GenericVector<S>::GenericVector(const S _x)
: dim(1)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   v[0] = _x;
   for (int i=1; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
GenericVector<S>::GenericVector()
: dim(0)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   for (int i=0; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
GenericVector<S>::GenericVector(const GenericVector<S> &o)
: dim(o.dim)
, x(v[0])
, y(v[1])
, z(v[2])
, w(v[3])
{
   memcpy(v, o.v, sizeof(v));
}

template<typename S>
GenericVector<S> &GenericVector<S>::operator=(const GenericVector<S> &rhs) {

   if (this != &rhs) {
      dim = rhs.dim;
      memcpy(v, rhs.v, sizeof(v));
   }

   return *this;
}

template<typename S>
GenericVector<S> GenericVector<S>::operator-() const {

   GenericVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] = -result[i];
   return result;
}

template<typename S>
GenericVector<S> GenericVector<S>::operator*(S const &rhs) const {

   GenericVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] *= rhs;
   return result;
}

template<typename S>
GenericVector<S> GenericVector<S>::operator+(GenericVector<S> const &rhs) const {

   GenericVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] += rhs[i];
   return result;
}

template<typename S>
GenericVector<S> GenericVector<S>::operator-(GenericVector const &rhs) const {

   GenericVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] -= rhs[i];
   return result;
}

template<typename S>
S GenericVector<S>::operator *(GenericVector const & rhs) const {

   S result = S();
   for (int i=0; i<dim; ++i)
      result += v[i] * rhs[i];
   return result;
}

template<typename S>
std::string GenericVector<S>::str() const {

   std::stringstream str;
   str << *this;
   return str.str();
}

} // namespace vistle

template<typename S>
std::ostream &operator<<(std::ostream &out, const vistle::GenericVector<S> &v) {

   out << "(";
   for (int i=0; i<v.dim; ++i) {
      if (i > 0)
         out << " ";
      out << v[i];
   }
   out << ")";
   return out;
}
#endif
