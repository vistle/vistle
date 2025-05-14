#ifndef VISTLE_CORE_PARAMVECTOR_IMPL_H
#define VISTLE_CORE_PARAMVECTOR_IMPL_H

#include <cassert>
#include <cstring>
#include <limits>
#include <sstream>

namespace std {
std::string to_string(const std::vector<std::string> &v)
{
    std::string str;
    for (const auto &s: v) {
        str += "<";
        str += s;
        str += ">";
    }
    return str;
}
} // namespace std

namespace vistle {

static_assert(MaxDimension >= 4, "ParameterVector does not support more than 4 arrays");

#define VINIT(d) dim(d), v(MaxDimension), x(v[0]), y(v[1]), z(v[2]), w(v[3])

template<typename S>
ParameterVector<S>::ParameterVector(const int dim, const S values[]): VINIT(dim)
{
    assert(dim <= MaxDimension);
    for (int i = 0; i < dim; ++i)
        v[i] = values[i];
    for (int i = dim; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const std::vector<S> &values): VINIT(values.size())
{
    assert(values.size() <= MaxDimension);
    for (unsigned i = 0; i < values.size(); ++i)
        v[i] = values[i];
    for (int i = dim; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x, const S _y, const S _z, const S _w): VINIT(4)
{
    v[0] = _x;
    v[1] = _y;
    v[2] = _z;
    v[3] = _w;
    for (int i = 4; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
template<int Dim>
ParameterVector<S>::ParameterVector(const ScalarVector<Dim> &_v): VINIT(Dim)
{
    static_assert(MaxDimension >= Dim, "Dim too large for ParameterVector");

    for (int i = 0; i < Dim; ++i)
        v[i] = _v[i];
    for (int i = Dim; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x, const S _y, const S _z): VINIT(3)
{
    v[0] = _x;
    v[1] = _y;
    v[2] = _z;
    for (int i = 3; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x, const S _y): VINIT(2)
{
    v[0] = _x;
    v[1] = _y;
    for (int i = 2; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const S _x): VINIT(1)
{
    v[0] = _x;
    for (int i = 1; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(): VINIT(0)
{
    for (int i = 0; i < MaxDimension; ++i)
        v[i] = S();
}

template<typename S>
ParameterVector<S>::ParameterVector(const ParameterVector<S> &o): VINIT(o.dim)
{
    std::copy(o.v.begin(), o.v.begin() + MaxDimension, v.begin());
}

template<typename S>
ParameterVector<S> &ParameterVector<S>::operator=(const ParameterVector<S> &rhs)
{
    if (this != &rhs) {
        dim = rhs.dim;
        std::copy(rhs.v.begin(), rhs.v.begin() + MaxDimension, v.begin());
    }

    return *this;
}

template<typename S>
ParameterVector<S>::ParameterVector(typename ParameterVector<S>::iterator from,
                                    typename ParameterVector<S>::iterator to)
: VINIT(MaxDimension)
{
    dim = 0;
    for (iterator it = from; it != to; ++it) {
        v[dim] = *it;
        ++dim;
    }
}
#undef VINIT

template<typename S>
ParameterVector<S> ParameterVector<S>::min(int dim)
{
    S v[MaxDimension];
    for (int i = 0; i < MaxDimension; ++i)
        v[i] = std::numeric_limits<S>::lowest();
    return ParameterVector<S>(dim, v);
}

template<typename S>
ParameterVector<S> ParameterVector<S>::max(int dim)
{
    S v[MaxDimension];
    for (int i = 0; i < MaxDimension; ++i)
        v[i] = std::numeric_limits<S>::max();
    return ParameterVector<S>(dim, v);
}
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

#if 0
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
#endif

template<typename S>
std::string ParameterVector<S>::str() const
{
    if (dim == 1) {
        return std::to_string(*this);
    }
    std::stringstream str;
    str << "(";
    for (auto i = 0; i < dim; ++i) {
        if (i > 0)
            str << " ";
        str << v[i];
    }
    str << ")";
    return str.str();
}

template<typename S>
bool operator==(const ParameterVector<S> &v1, const ParameterVector<S> &v2)
{
    if (v1.dim != v2.dim)
        return false;
    for (int i = 0; i < v1.dim; ++i)
        if (v1[i] != v2[i])
            return false;

    return true;
}

template<typename S>
bool operator!=(const ParameterVector<S> &v1, const ParameterVector<S> &v2)
{
    return !(v1 == v2);
}

template<typename S>
bool operator<(const ParameterVector<S> &v1, const ParameterVector<S> &v2)
{
    for (int i = 0; i < v1.dim && i < v2.dim; ++i) {
        if (v1[i] > v2[i])
            return false;
        if (v1[i] < v2[i])
            return true;
    }

    return v1.dim < v2.dim;
}

template<typename S>
bool operator>(const ParameterVector<S> &v1, const ParameterVector<S> &v2)
{
    for (int i = 0; i < v1.dim && i < v2.dim; ++i) {
        if (v1[i] < v2[i])
            return false;
        if (v1[i] > v2[i])
            return true;
    }

    return v1.dim > v2.dim;
}

template<typename S>
std::ostream &operator<<(std::ostream &out, const ParameterVector<S> &v)
{
    out << "(";
    for (int i = 0; i < v.dim; ++i) {
        if (i > 0)
            out << " ";
        out << v[i];
    }
    out << ")";
    return out;
}

} // namespace vistle
#endif
