#ifndef VECTOR_H
#define VECTOR_H

#include "scalar.h"

namespace vistle {

class Vector {
public:
   Vector(const Scalar x, const Scalar y, const Scalar z)
   : x(x), y(y), z(z) {}
   Vector() : x(Scalar()), y(Scalar()), z(Scalar()) {}

   // negate
   Vector operator-() const {
      return Vector(-x, -y, -z);
   }

   // scaling
   Vector operator*(Scalar const & rhs) const {
      return Vector(rhs * x, rhs * y, rhs * z);
   }

   // vector addition
   Vector operator+(Vector const & rhs) const {
      return Vector(x + rhs.x, y + rhs.y, z + rhs.z);
   }

   // vector substraction
   Vector operator-(Vector const & rhs) const {
      return Vector(x - rhs.x, y - rhs.y, z - rhs.z);
   }

   // scalar product
   Scalar operator*(Vector const & rhs) const {
      return x * rhs.x + y * rhs.y + z * rhs.z;
   }

   Scalar x, y, z;
};

} // namespace vistle

#endif
