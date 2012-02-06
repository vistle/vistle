#ifndef VECTOR_H
#define VECTOR_H

namespace vistle {

class Vector {
public:
   Vector(const float x, const float y, const float z);
   Vector();

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

   float x, y, z;
};

} // namespace vistle

#endif
