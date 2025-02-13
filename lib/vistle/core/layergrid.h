#ifndef VISTLE_CORE_LAYERGRID_H
#define VISTLE_CORE_LAYERGRID_H

//-------------------------------------------------------------------------
// LayerGrid: structured grid that stacks layers of varying height (z), but is uniform in x- and y-dimensions
//-------------------------------------------------------------------------

#include "scalar.h"
#include "vectortypes.h"
#include "shm.h"
#include "structuredgridbase.h"
#include "vec.h"
#include "normals.h"
#include "celltree.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT LayerGrid: public Vec<Scalar, 1>,
                              virtual public StructuredGridBase,
                              virtual public CelltreeInterface<3> {
    V_OBJECT(LayerGrid);

public:
    typedef Vec<Scalar> Base;
    typedef typename CelltreeInterface<3>::Celltree Celltree;

    LayerGrid(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const Meta &meta = Meta());

    // get functions for metadata
    Index getNumDivisions(int c) override { return d()->numDivisions[c]; }
    Index getNumDivisions(int c) const override { return m_numDivisions[c]; }
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) override;
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const override;
    Index getGlobalIndexOffset(int c) const override { return d()->indexOffset[c]; }
    void setGlobalIndexOffset(int c, Index offset) override;

    // virtual set functions
    void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) override;

    // get/set functions for shared memory members
    Scalar64 *min() { return d()->min; }
    Scalar64 *max() { return d()->max; }
    const Scalar *min() const { return m_min; }
    const Scalar *max() const { return m_max; }
    const Scalar *dist() const { return m_dist; }
    array &z() { return x(); }
    const ShmArrayProxy<Scalar> &z() const { return x(); }

    // GridInterface
    Index getNumVertices() override;
    Index getNumVertices() const override;
    std::pair<Vector3, Vector3> getBounds() const override;
    Normals::const_ptr normals() const override;
    void setNormals(Normals::const_ptr normals) override;
    std::pair<Vector3, Vector3> cellBounds(Index elem) const override;
    std::vector<Vector3> cellCorners(Index elem) const;
    Index findCell(const Vector3 &point, Index hint = InvalidIndex, int flags = NoFlags) const override;
    bool inside(Index elem, const Vector3 &point) const override;
    Interpolator getInterpolator(Index elem, const Vector3 &point, DataBase::Mapping mapping = DataBase::Vertex,
                                 InterpolationMode mode = Linear) const override;

    bool hasCelltree() const override;
    Celltree::const_ptr getCelltree() const override;
    bool validateCelltree() const override;
    Scalar exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const override;
    Vector3 getVertex(Index v) const override;

    void copyAttributes(Object::const_ptr src, bool replace = true) override;

    void updateInternals() override;
    std::set<Object::const_ptr> referencedObjects() const override;

private:
    // mutable pointers to ShmVectors
    mutable Scalar m_min[2];
    mutable Scalar m_max[2];
    mutable Scalar m_dist[2];
    mutable Index m_numDivisions[3];
    mutable Index m_ghostLayers[3][2];
    mutable Celltree::const_ptr m_celltree;

    void createCelltree(Index dims[3]) const;

    // data object
    V_DATA_BEGIN(LayerGrid);

    shm_obj_ref<Normals> normals;
    Scalar64 min[2]; //< coordinates of minimum grid point
    Scalar64 max[2]; //< coordinates of maximum grid point
    Index64 indexOffset[3]; //< global index offset
    Index64 numDivisions[3]; //< number of divisions on each axis (1 more than number of cells)
    Index64 ghostLayers[3][2]; //< number of ghost cell layers in each of x, y, z directions

    Data(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const std::string &name,
         const Meta &meta = Meta());
    ~Data();
    static Data *create(const size_t numVert_x = 0, const size_t numVert_y = 0, const size_t numVert_z = 0,
                        const Meta &meta = Meta());

    V_DATA_END(LayerGrid);
};

} // namespace vistle
#endif
