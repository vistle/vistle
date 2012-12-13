#include "vec.h"

namespace vistle {

V_SERIALIZERS4(Vec<T,Dim>, template<class T,int Dim>);

#define VEC(T,D,ID) \
   template<> const Object::Type Vec<T,D>::s_type = Object::ID; \
   V_OBJECT_TYPE4(Vec<T,D>, ID, Object::ID);

VEC(Scalar, 1, VECFLOAT)
VEC(char, 1, VECCHAR)
VEC(int, 1, VECINT)

VEC(Scalar, 3, VEC3FLOAT)
VEC(char, 3, VEC3CHAR)
VEC(int, 3, VEC3INT)

template<class V>
static void inst_vec() {
   new V(0, -1, -1);
};

void inst_vecs() {
   inst_vec<Vec<char,1> >();
   inst_vec<Vec<int,1> >();
   inst_vec<Vec<Scalar,1> >();

#if 0
   inst_vec<Vec<char,2> >();
   inst_vec<Vec<int,2> >();
   inst_vec<Vec<Scalar,2> >();
#endif

   inst_vec<Vec<char,3> >();
   inst_vec<Vec<int,3> >();
   inst_vec<Vec<Scalar,3> >();
}

} // namespace vistle
