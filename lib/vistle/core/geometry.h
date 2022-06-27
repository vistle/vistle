#ifndef VISTLE_GEOMETRY_H
#define VISTLE_GEOMETRY_H

#include "export.h"
#include "object.h"
#include "vectortypes.h"

namespace vistle {

class Normals;
typedef std::shared_ptr<Normals> normals_ptr;
typedef std::shared_ptr<const Normals> normals_const_ptr;

class V_COREEXPORT GeometryInterface: virtual public ObjectInterfaceBase {
public:
    virtual std::pair<Vector3, Vector3> getBounds() const = 0;
    virtual Index getNumVertices() = 0;
    virtual Index getNumVertices() const = 0;
    virtual normals_const_ptr normals() const = 0;
    virtual Vector3 getVertex(Index v) const = 0;
};

class V_COREEXPORT ElementInterface: virtual public GeometryInterface {
public:
    virtual Index getNumElements() = 0;
    virtual Index getNumElements() const = 0;
    virtual Index cellNumFaces(Index elem) const = 0;
    virtual std::vector<Index> cellVertices(Index elem) const = 0;
};

} // namespace vistle
#endif
