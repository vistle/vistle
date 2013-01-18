#ifndef VECTOR_H
#define VECTOR_H

#include <ostream>
#include "scalar.h"
#include "dimensions.h"

namespace vistle {

template<typename S>
class GenericVector {
public:
   typedef S Scalar;

   GenericVector(const int dim, const S values[]);
   GenericVector(const S x, const S y, const S z, const S w);
   GenericVector(const S x, const S y, const S z);
   GenericVector(const S x, const S y);
   GenericVector(const S x);
   GenericVector();
   GenericVector(const GenericVector &other);

   GenericVector &operator=(const GenericVector &rhs);

   int dim;
   S &x, &y, &z, &w;
   S v[MaxDimension];

   // negate
   GenericVector operator-() const;

   // scalar multiplication
   GenericVector operator*(S const & rhs) const;

   // vector addition
   GenericVector operator+(GenericVector const & rhs) const;

   // vector supstraction
   GenericVector operator-(GenericVector const & rhs) const;

   // scalar product
   S operator*(GenericVector const & rhs) const;

   S &operator[](int i) { return v[i]; }
   const S &operator[](int i) const { return v[i]; }

   operator S *() { return v; }
   operator const S *() const { return v; }

   std::string str() const;
};

typedef GenericVector<Scalar> ScalarVector;
typedef ScalarVector Vector;

} // namespace vistle

template<typename S>
std::ostream &operator<<(std::ostream &out, const vistle::GenericVector<S> &v);
#endif // VECTOR_H


#ifdef VISTLE_VECTOR_IMPL
#include "vector_impl.h"
#endif
