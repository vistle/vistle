//-------------------------------------------------------------------------
// UNIFORM GRID CLASS H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------
#ifndef UNIFORM_GRID_H
#define UNIFORM_GRID_H

#include "scalar.h"
#include "shm.h"
#include "shmvector.h"
#include "structuredgridbase.h"
#include "normals.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF UNIFORMGRID
//-------------------------------------------------------------------------
class V_COREEXPORT UniformGrid: public Object, public StructuredGridBase {
    V_OBJECT(UniformGrid);

public:
    typedef Object Base;

    // constructor
    UniformGrid(size_t xdim, size_t ydim, size_t zdim, const Meta &meta = Meta());

    std::set<Object::const_ptr> referencedObjects() const override;

    // get/set functions
    Index getNumDivisions(int c) override { return d()->numDivisions[c]; }
    Index getNumDivisions(int c) const override { return m_numDivisions[c]; }
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) override;
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const override;
    Index getGlobalIndexOffset(int c) const override { return d()->indexOffset[c]; }
    void setGlobalIndexOffset(int d, Index offset) override;

    // virtual set functions
    void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) override;

    // get/set functions for shared memory members
    Scalar *min() { return d()->min; }
    Scalar *max() { return d()->max; }
    const Scalar *min() const { return m_min; }
    const Scalar *max() const { return m_max; }
    const Scalar *dist() const { return m_dist; }

    // GeometryInterface
    std::pair<Vector3, Vector3> getBounds() const override;
    Normals::const_ptr normals() const override;
    void setNormals(Normals::const_ptr normals) override;

    // GridInterface
    Index getNumVertices() override;
    Index getNumVertices() const override;
    std::pair<Vector3, Vector3> cellBounds(Index elem) const override;
    Index findCell(const Vector3 &point, Index hint = InvalidIndex, int flags = NoFlags) const override;
    bool inside(Index elem, const Vector3 &point) const override;
    Interpolator getInterpolator(Index elem, const Vector3 &point, DataBase::Mapping mapping = DataBase::Vertex,
                                 InterpolationMode mode = Linear) const override;
    Scalar exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const override;
    Vector3 getVertex(Index v) const override;

    void copyAttributes(Object::const_ptr src, bool replace = true) override;

private:
    // mutable pointers to ShmVectors
    mutable Index m_size;
    mutable Index m_numDivisions[3];
    mutable Scalar m_min[3];
    mutable Scalar m_max[3];
    mutable Scalar m_dist[3];
    mutable Index m_ghostLayers[3][2];

    // data object
    V_DATA_BEGIN(UniformGrid);

    shm_obj_ref<Normals> normals;
    // each of the following variables represents a coordinate (by index, in order x, y, z)
    Index indexOffset[3]; //< global index offset
    Index numDivisions[3]; //< number of divisions on each axis (1 more than number of cells)
    Scalar min[3]; //< coordinates of minimum grid point
    Scalar max[3]; //< coordinates of maximum grid point
    Index ghostLayers[3][2]; //< number of ghost cell layers in each x, y, z directions

    Data(const std::string &name, size_t xdim, size_t ydim, size_t zdim, const Meta &meta = Meta());
    ~Data();
    static Data *create(size_t xdim = 0, size_t ydim = 0, size_t zdim = 0, const Meta &meta = Meta());

    V_DATA_END(UniformGrid);
};

} // namespace vistle
#endif /* UNIFORM_GRID_H */
