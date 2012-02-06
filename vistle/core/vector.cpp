#include "vector.h"

namespace vistle {

Vector::Vector(const float _x, const float _y, const float _z)
   : x(_x), y(_y), z(_z) {

}

Vector::Vector(): x(0), y(0), z(0) {

}

Vector Vector::operator - () const {

   return Vector(-x, -y, -z);
}

Vector Vector::operator * (float const & rhs) const {

   return Vector(rhs * x, rhs * y, rhs * z);
}


Vector Vector::operator + (Vector const & rhs) const {

   return Vector(x + rhs.x, y + rhs.y, z + rhs.z);
}

Vector Vector::operator - (Vector const & rhs) const {

   return Vector(x - rhs.x, y - rhs.y, z - rhs.z);
}

float Vector::operator * (Vector const & rhs) const {

      return x * rhs.x + y * rhs.y + z * rhs.z;
}

} // namespace vistle
