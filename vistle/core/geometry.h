#ifndef VISTLE_GEOMETRY_H
#define VISTLE_GEOMETRY_H

#include "object.h"
#include "vector.h"
#include "database.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT GeometryInterface {
 public:
   virtual std::pair<Vector, Vector> getBounds() const = 0;
};

}
#endif
