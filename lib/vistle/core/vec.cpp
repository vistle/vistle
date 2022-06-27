#include "vec.h"
#include "scalars.h"
#include <cassert>
#include "vec_template.h"
#include "vec_impl.h"
#include "archives.h"
#include <vistle/util/exception.h>

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

FOR_ALL_SCALARS(V_VEC_TEMPLATE_INST)

#undef V_VEC_TEMPLATE_INST

} // namespace vistle
