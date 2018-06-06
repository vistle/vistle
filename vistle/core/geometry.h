#ifndef VISTLE_GEOMETRY_H
#define VISTLE_GEOMETRY_H

#include "export.h"
#include "object.h"
#include "vector.h"
#include "normals.h"

namespace vistle {

class V_COREEXPORT GeometryInterface: virtual public ObjectInterfaceBase {
 public:
   virtual std::pair<Vector, Vector> getBounds() const = 0;
   //virtual Index getNumElements() const = 0;
   virtual Index getNumVertices() const = 0;
   virtual Normals::const_ptr normals() const = 0;
};

class V_COREEXPORT ElementInterface: virtual public GeometryInterface {
 public:
   virtual Index getNumElements() const = 0;
};

}
#endif
