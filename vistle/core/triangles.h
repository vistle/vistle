#ifndef VISTLE_TRIANGLES_H
#define VISTLE_TRIANGLES_H

#include "ngons.h"

namespace vistle {

extern template class V_COREEXPORT Ngons<3>;
typedef Ngons<3> Triangles;
V_OBJECT_DECL(Ngons<3>)

} // namespace vistle
#endif
