#ifndef VISTLE_QUADS_H
#define VISTLE_QUADS_H

#include "ngons.h"

namespace vistle {

extern template class V_COREEXPORT Ngons<4>;
typedef Ngons<4> Quads;
V_OBJECT_DECL(Ngons<4>)
} // namespace vistle
#endif
