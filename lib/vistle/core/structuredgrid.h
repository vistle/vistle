#ifndef VISTLE_CORE_STRUCTUREDGRID_H
#define VISTLE_CORE_STRUCTUREDGRID_H

//-------------------------------------------------------------------------
// * Structured Grid Container Object
//-------------------------------------------------------------------------

#include "scalar.h"
#include "shm.h"
#include "structuredgridbase.h"
#include "coords.h"
#include "celltree.h"
#include "normals.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF STRUCTUREDGRID
//-------------------------------------------------------------------------
class V_COREEXPORT StructuredGrid: public Coords,
                                   virtual public StructuredGridBase,
                                   virtual public CelltreeInterface<3> {
    V_OBJECT(StructuredGrid);

public:
    typedef Coords Base;
    typedef typename CelltreeInterface<3>::Celltree Celltree;

    // constructor
    StructuredGrid(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const Meta &meta = Meta());

    // get functions for metadata
    Index getNumDivisions(int c) override { return d()->numDivisions[c]; }
    Index getNumDivisions(int c) const override { return m_numDivisions[c]; }
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) override;
    Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const override;
    Index getGlobalIndexOffset(int c) const override { return d()->indexOffset[c]; }
    void setGlobalIndexOffset(int c, Index offset) override;

    // virtual set functions
    void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) override;

    // GridInterface
    Index getNumVertices() const override;
    std::pair<Vector3, Vector3> getBounds() const override;
    Normals::const_ptr normals() const override;
    void setNormals(Normals::const_ptr normals) override;
    std::pair<Vector3, Vector3> cellBounds(Index elem) const override;
    Index findCell(const Vector3 &point, Index hint = InvalidIndex, int flags = NoFlags) const override;
    bool inside(Index elem, const Vector3 &point) const override;
    Interpolator getInterpolator(Index elem, const Vector3 &point, DataBase::Mapping mapping = DataBase::Vertex,
                                 InterpolationMode mode = Linear) const override;

    bool hasCelltree() const override;
    Celltree::const_ptr getCelltree() const override;
    bool validateCelltree() const override;
    Scalar exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const override;

    void copyAttributes(Object::const_ptr src, bool replace = true) override;

    void updateInternals() override;
    std::set<Object::const_ptr> referencedObjects() const override;

private:
    // mutable pointers to ShmVectors
    mutable Index m_numDivisions[3];
    mutable Index m_ghostLayers[3][2];
    mutable Celltree::const_ptr m_celltree;

    void createCelltree(Index dims[3]) const;

    // data object
    V_DATA_BEGIN(StructuredGrid);

    shm_obj_ref<Normals> normals;
    Index64 indexOffset[3]; //< global index offset
    Index64 numDivisions[3]; //< number of divisions on each axis (1 more than number of cells)
    Index64 ghostLayers[3][2]; //< number of ghost cell layers in each of x, y, z directions

    Data(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const std::string &name,
         const Meta &meta = Meta());
    ~Data();
    static Data *create(const size_t numVert_x = 0, const size_t numVert_y = 0, const size_t numVert_z = 0,
                        const Meta &meta = Meta());

    V_DATA_END(StructuredGrid);
};

} // namespace vistle
#endif
