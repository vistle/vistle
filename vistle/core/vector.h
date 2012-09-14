#ifndef VECTOR_H
#define VECTOR_H

#include "scalar.h"

namespace vistle {

class Vector {
public:
   Vector(const Scalar x, const Scalar y, const Scalar z);
   Vector();

   // negate
   Vector operator - () const;

   // scalar multiplication
   Vector operator * (Scalar const & rhs) const;

   // vector addition
   Vector operator + (Vector const & rhs) const;

   // vector supstraction
   Vector operator - (Vector const & rhs) const;

   // scalar product
   Scalar operator * (Vector const & rhs) const;

   Scalar x, y, z;
};

} // namespace vistle

#endif
