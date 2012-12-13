#include "vec.h"

namespace vistle {

V_SERIALIZERS4(Vec<T,Dim>, template<class T,int Dim>);

#define VEC(T,D,ID) \
   template<> const Object::Type Vec<T,D>::s_type = Object::ID; \
   V_OBJECT_TYPE4(Vec<T,D>, ID, Object::ID);

VEC(float, 1, VECFLOAT)
VEC(double, 1, VECDOUBLE)
VEC(char, 1, VECCHAR)
VEC(int, 1, VECINT)

#if 0
VEC(float, 2, VEC2FLOAT)
VEC(double, 2, VEC2DOUBLE)
VEC(char, 2, VEC2CHAR)
VEC(int, 2, VEC2INT)
#endif

VEC(float, 3, VEC3FLOAT)
VEC(double, 3, VEC3DOUBLE)
VEC(char, 3, VEC3CHAR)
VEC(int, 3, VEC3INT)

#if 0
VEC(float, 4, VEC4FLOAT)
VEC(double, 4, VEC4DOUBLE)
VEC(char, 4, VEC4CHAR)
VEC(int, 4, VEC4INT)
#endif

template<class V>
static void inst_vec() {
   new V(0, -1, -1);
};

#define INST_VECS(d) \
   inst_vec<Vec<char,d> >(); \
   inst_vec<Vec<int,d> >(); \
   inst_vec<Vec<float,d> >(); \
   inst_vec<Vec<double,d> >();


void inst_vecs() {
   INST_VECS(1)
   //INST_VECS(2)
   INST_VECS(3)
   //INST_VECS(4)
}

} // namespace vistle
