#include "vec.h"

namespace vistle {

V_SERIALIZERS2(Vec<T>, template<class T>);

template<> const Object::Type Vec<Scalar>::s_type  = Object::VECFLOAT;
template<> const Object::Type Vec<int>::s_type    = Object::VECINT;
template<> const Object::Type Vec<char>::s_type   = Object::VECCHAR;

V_OBJECT_TYPE3(Vec<char>, Vec_char, Object::VECCHAR);
V_OBJECT_TYPE3(Vec<int>, Vec_int, Object::VECINT);
V_OBJECT_TYPE3(Vec<Scalar>, Vec_Scalar, Object::VECFLOAT);

} // namespace vistle
