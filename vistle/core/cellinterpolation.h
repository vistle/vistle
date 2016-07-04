#ifndef VISTLE_CELLINTERPOLATION_H
#define VISTLE_CELLINTERPOLATION_H

#include "export.h"
#include "vector.h"

namespace vistle {

V_COREEXPORT Vector trilinearInverse(const Vector &p0, const Vector p[8]);

}
#endif
