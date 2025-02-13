#ifndef VISTLE_CORE_COORDS_H
#define VISTLE_CORE_COORDS_H

#include "scalar.h"
#include "shm.h"
#include "object.h"
#include "vec.h"
#include "geometry.h"
#include "normals.h"
#include "shm_obj_ref.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT Coords: public Vec<Scalar, 3>, virtual public GeometryInterface {
    V_OBJECT(Coords);

public:
    typedef Vec<Scalar, 3> Base;

    Coords(const size_t numVertices, const Meta &meta = Meta());

    void resetCoords(); //< remove reference to coordinates and create empty ones
    std::set<Object::const_ptr> referencedObjects() const override;

    std::pair<Vector3, Vector3> getBounds() const override;
    Index getNumCoords();
    Index getNumCoords() const;
    Index getNumVertices() override;
    Index getNumVertices() const override;
    Normals::const_ptr normals() const override;
    void setNormals(Normals::const_ptr normals);
    Vector3 getVertex(Index v) const override;

    V_DATA_BEGIN(Coords);
    shm_obj_ref<Normals> normals;

    Data(const Index numVertices = 0, Type id = UNKNOWN, const std::string &name = "", const Meta &meta = Meta());
    Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id);
    ~Data();
    static Data *create(const std::string &name = "", Type id = UNKNOWN, const Index numVertices = 0,
                        const Meta &meta = Meta());

    V_DATA_END(Coords);
};

V_OBJECT_DECL(Coords)

//ARCHIVE_ASSUME_ABSTRACT(Coords)

} // namespace vistle
#endif
