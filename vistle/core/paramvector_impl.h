#ifndef VECTOR_IMPL_H
#define VECTOR_IMPL_H

#include <sstream>
#include <cassert>
#include <limits>
#include <boost/static_assert.hpp>

namespace vistle {

BOOST_STATIC_ASSERT(MaxDimension >= 4);

#define VINIT(d) \
  dim(d) \
, v(MaxDimension) \
, m_min(MaxDimension) \
, m_max(MaxDimension) \
, x(v[0]) \
, y(v[1]) \
, z(v[2]) \
, w(v[3])

#define VMINMAX(T) \
  for(int i=0; i<MaxDimension; ++i) { \
     m_min[i] = std::numeric_limits<T>::max() > 0 ? -std::numeric_limits<T>::max() : std::numeric_limits<T>::min(); \
     m_max[i] = std::numeric_limits<T>::max(); \
  }

template<typename S>
ParameterVector<S>::ParameterVector(const int dim, const S values[])
: VINIT(dim)
{
   VMINMAX(S);
   assert(dim <= MaxDimension);
   for (int i=0; i<dim; ++i)
      v[i] = values[i];
   for (int i=dim; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x, const S _y, const S _z, const S _w)
: VINIT(4)
{
   VMINMAX(S);
   v[0] = _x;
   v[1] = _y;
   v[2] = _z;
   v[3] = _w;
   for (int i=4; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x, const S _y, const S _z)
: VINIT(3)
{
   VMINMAX(S);
   v[0] = _x;
   v[1] = _y;
   v[2] = _z;
   for (int i=3; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x, const S _y)
: VINIT(2)
{
   VMINMAX(S);
   v[0] = _x;
   v[1] = _y;
   for (int i=2; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x)
: VINIT(1)
{
   VMINMAX(S);
   v[0] = _x;
   for (int i=1; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector()
: VINIT(0)
{
   VMINMAX(S);
   for (int i=0; i<MaxDimension; ++i)
      v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const ParameterVector<S> &o)
: VINIT(o.dim)
{
   memcpy(&v[0], &o.v[0], sizeof(v[0])*MaxDimension);
   memcpy(&m_min[0], &o.m_min[0], sizeof(m_min[0])*MaxDimension);
   memcpy(&m_max[0], &o.m_max[0], sizeof(m_max[0])*MaxDimension);
}

template<typename S>
ParameterVector<S> &ParameterVector<S>::operator=(const ParameterVector<S> &rhs) {

   if (this != &rhs) {
      dim = rhs.dim;
      memcpy(&v[0], &rhs.v[0], sizeof(v[0])*MaxDimension);
      for (int i=0; i<dim; ++i) {
         if (v[i] < m_min[i])
            v[i] = m_min[i];
         if (v[i] > m_max[i])
            v[i] = m_max[i];
      }
#if 0
      memcpy(&m_min[0], &rhs.m_min[0], sizeof(m_min[0])*MaxDimension);
      memcpy(&m_max[0], &rhs.m_max[0], sizeof(m_max[0])*MaxDimension);
#endif
   }

   return *this;
}

template<typename S>
ParameterVector<S>::ParameterVector(ParameterVector<S>::iterator from, ParameterVector<S>::iterator to)
: VINIT(MaxDimension)
{
   VMINMAX(S);
   dim = 0;
   for (iterator it=from; it!=to; ++it) {
      v[dim] = *it;
      ++dim;
   }
}
#undef VINIT

#if 0
template<typename S>
ParameterVector<S> ParameterVector<S>::operator-() const {

   ParameterVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] = -result[i];
   return result;
}

template<typename S>
ParameterVector<S> ParameterVector<S>::operator*(S const &rhs) const {

   ParameterVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] *= rhs;
   return result;
}

template<typename S>
ParameterVector<S> ParameterVector<S>::operator+(ParameterVector<S> const &rhs) const {

   ParameterVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] += rhs[i];
   return result;
}

template<typename S>
ParameterVector<S> ParameterVector<S>::operator-(ParameterVector const &rhs) const {

   ParameterVector result(*this);
   for (int i=0; i<dim; ++i)
      result[i] -= rhs[i];
   return result;
}

template<typename S>
S ParameterVector<S>::operator *(ParameterVector const & rhs) const {

   S result = S();
   for (int i=0; i<dim; ++i)
      result += v[i] * rhs[i];
   return result;
}
#endif

template<typename S>
void ParameterVector<S>::setMinimum(const S x, const S y, const S z, const S w) {
   m_min[0] = x;
   m_min[1] = y;
   m_min[2] = z;
   m_min[3] = w;
}

template<typename S>
void ParameterVector<S>::setMinimum(const S *v) {
   for (int i=0; i<dim; ++i)
      m_min[i] = v[i];
}

template<typename S>
ParameterVector<S> ParameterVector<S>::minimum() const {

   return ParameterVector(dim, &m_min[0]);
}

template<typename S>
void ParameterVector<S>::setMaximum(const S x, const S y, const S z, const S w) {
   m_max[0] = x;
   m_max[1] = y;
   m_max[2] = z;
   m_max[3] = w;
}

template<typename S>
void ParameterVector<S>::setMaximum(const S *v) {
   for (int i=0; i<dim; ++i)
      m_max[i] = v[i];
}

template<typename S>
ParameterVector<S> ParameterVector<S>::maximum() const {

   return ParameterVector(dim, &m_max[0]);
}

template<typename S>
std::string ParameterVector<S>::str() const {

   std::stringstream str;
   str << *this;
   return str.str();
}

} // namespace vistle

template<typename S>
std::ostream &operator<<(std::ostream &out, const vistle::ParameterVector<S> &v) {

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
