//-------------------------------------------------------------------------
// RECTILINEAR GRID CLASS H
// *
// * Rectilinear Grid Container Object
//-------------------------------------------------------------------------
#ifndef RECTILINEAR_GRID_H
#define RECTILINEAR_GRID_H

#include "scalar.h"
#include "shm.h"
#include "shmvector.h"
#include "structuredgridbase.h"
#include "normals.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF RECTILINEARGRID
//-------------------------------------------------------------------------
class V_COREEXPORT RectilinearGrid: public Object, virtual public StructuredGridBase {
    V_OBJECT(RectilinearGrid);

public:
    typedef Object Base;

    // constructor
    RectilinearGrid(const Index numDivX, const Index numDivY, const Index numDivZ, const Meta &meta = Meta());

    // get functions for metadata
    Index getNumDivisions(int c) override { return d()->coords[c]->size(); }
    Index getNumDivisions(int c) const override { return m_numDivisions[c]; }
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) override;
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const override;
    Index getGlobalIndexOffset(int c) const override { return d()->indexOffset[c]; }
    void setGlobalIndexOffset(int c, Index offset) override;

    // virtual set functions
    void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) override;

    // get/set functions for shared memory members
    shm<Scalar>::array &coords(int c) { return *d()->coords[c]; }
    const Scalar *coords(int c) const { return m_coords[c]; }

    // GridInterface
    Index getNumVertices() override;
    Index getNumVertices() const override;
    std::pair<Vector3, Vector3> getBounds() const override;
    Normals::const_ptr normals() const override;
    void setNormals(Normals::const_ptr normals) override;
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
    mutable Index m_numDivisions[3];
    mutable const Scalar *m_coords[3];
    mutable Index m_ghostLayers[3][2];
    mutable Index m_size = 0;

    // data object
    V_DATA_BEGIN(RectilinearGrid);

    shm_obj_ref<Normals> normals;
    Index indexOffset[3]; //< global index offset
    ShmVector<Scalar> coords[3]; //< coordinates of divisions in x, y, and z
    Index ghostLayers[3][2]; //< number of ghost cell layers in each x, y, z directions

    Data(const Index numDivX, const Index numDivY, const Index numDivZ, const std::string &name,
         const Meta &meta = Meta());
    ~Data();
    static Data *create(const Index numDivX = 0, const Index numDivY = 0, const Index numDivZ = 0,
                        const Meta &meta = Meta());

    V_DATA_END(RectilinearGrid);
};

} // namespace vistle
#endif /* RECTILINEAR_GRID_H */
