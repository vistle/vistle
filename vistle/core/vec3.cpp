#include "vec3.h"

namespace vistle {

template<> const Object::Type Vec3<Scalar>::s_type = Object::VEC3FLOAT;
template<> const Object::Type Vec3<int>::s_type   = Object::VEC3INT;
template<> const Object::Type Vec3<char>::s_type  = Object::VEC3CHAR;

V_OBJECT_TYPE3(Vec3<char>, Vec3_char, Object::VEC3CHAR);
V_OBJECT_TYPE3(Vec3<int>, Vec3_int, Object::VEC3INT);
V_OBJECT_TYPE3(Vec3<Scalar>, Vec3_Scalar, Object::VEC3FLOAT);

} // namespace vistle
