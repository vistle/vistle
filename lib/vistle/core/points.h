#ifndef VISTLE_CORE_POINTS_H
#define VISTLE_CORE_POINTS_H


#include "coords.h"

namespace vistle {

class V_COREEXPORT Points: public Coords, virtual public ElementInterface {
    V_OBJECT(Points);

public:
    typedef Coords Base;
    std::set<Object::const_ptr> referencedObjects() const override;

    Points(size_t numPoints, const Meta &meta = Meta());

    Index getNumPoints();
    Index getNumPoints() const;

    Vec<Scalar>::const_ptr radius() const;
    void setRadius(Vec<Scalar>::const_ptr radius);

    Index getNumElements() override;
    Index getNumElements() const override;
    Index cellNumFaces(Index elem) const override;
    Index cellNumVertices(Index elem) const override;
    std::vector<Index> cellVertices(Index elem) const override;
    Vector3 cellCenter(Index elem) const override;

    V_DATA_BEGIN(Points);
    shm_obj_ref<Vec<Scalar>> radius;

    Data(const size_t numPoints = 0, const std::string &name = "", const Meta &meta = Meta());
    Data(const Vec<Scalar, 3>::Data &o, const std::string &n);
    static Data *create(const size_t numPoints = 0, const Meta &meta = Meta());

    V_DATA_END(Points);
};

} // namespace vistle
#endif
