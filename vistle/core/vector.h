#ifndef VECTOR_H
#define VECTOR_H

#include "scalar.h"

namespace vistle {

class Vector {
public:
   Vector(const Scalar x, const Scalar y, const Scalar z)
   : x(v[0]), y(v[1]), z(v[2]) {
      v[0] = x;
      v[1] = y;
      v[2] = z;
   }
   Vector()
   : x(v[0]), y(v[1]), z(v[2]) {
      v[0] = v[1] = v[2] = 0.;
   }
   Vector(const Vector &o)
   : x(v[0]), y(v[0]), z(v[2]) {
      v[0] = o.v[0];
      v[1] = o.v[1];
      v[2] = o.v[2];
   }

   Vector &operator=(const Vector &rhs) {
      v[0] = rhs.v[0];
      v[1] = rhs.v[1];
      v[2] = rhs.v[2];
      return *this;
   }

   Scalar &operator[](int idx) { return v[idx]; }
   const Scalar &operator[](int idx) const { return v[idx]; }

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

   Scalar v[3];
   Scalar &x, &y, &z;
};

} // namespace vistle

#endif
