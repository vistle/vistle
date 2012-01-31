#ifndef VECTOR_H
#define VECTOR_H

namespace vistle {

namespace util {

class Vector {
public:
   Vector(const float x, const float y, const float z);

   // negate
   Vector operator - () const;

   // scalar multiplication
   Vector operator * (float const & rhs) const;

   // vector addition
   Vector operator + (Vector const & rhs) const;

   // vector supstraction
   Vector operator - (Vector const & rhs) const;

   // scalar product
   float operator * (Vector const & rhs) const;

   static Vector lerp(const Vector & a, const Vector & b);

   float x, y, z;
};

} // namespace util
} // namespace vistle

#endif
