#include "vec.h"
#include "scalars.h"
#include <cassert>
#include "vec_template.h"
#include "vec_impl.h"
#include "archives.h"

namespace vistle {

#define COMMA ,

#define V_VEC_TEMPLATE_INST(ValueType) \
    template class Vec<ValueType, 1>; \
    V_OBJECT_INST(Vec<ValueType COMMA 1>) \
    template class Vec<ValueType, 2>; \
    V_OBJECT_INST(Vec<ValueType COMMA 2>) \
    template class Vec<ValueType, 3>; \
    V_OBJECT_INST(Vec<ValueType COMMA 3>)
/* template class Vec<ValueType,4>; \
    V_OBJECT_INST(Vec<ValueType COMMA 4>) \
    */

V_VEC_TEMPLATE_INST(char)
V_VEC_TEMPLATE_INST(signed char)
V_VEC_TEMPLATE_INST(unsigned char)
V_VEC_TEMPLATE_INST(int32_t)
V_VEC_TEMPLATE_INST(uint32_t)
V_VEC_TEMPLATE_INST(int64_t)
V_VEC_TEMPLATE_INST(uint64_t)
V_VEC_TEMPLATE_INST(float)
V_VEC_TEMPLATE_INST(double)
#undef V_VEC_TEMPLATE_INST

} // namespace vistle
