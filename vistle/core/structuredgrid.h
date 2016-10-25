//-------------------------------------------------------------------------
// STRUCTURED GRID CLASS H
// *
// * Structured Grid Container Object
//-------------------------------------------------------------------------
#ifndef STRUCTURED_GRID_H
#define STRUCTURED_GRID_H

#include "scalar.h"
#include "shm.h"
#include "structuredgridbase.h"
#include "coords.h"
#include "celltree.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF STRUCTUREDGRID
//-------------------------------------------------------------------------
class V_COREEXPORT StructuredGrid: public Coords, virtual public StructuredGridBase, virtual public CelltreeInterface<3> {
   V_OBJECT(StructuredGrid);

public:
   typedef Coords Base;
   typedef typename CelltreeInterface<3>::Celltree Celltree;

   // constructor
   StructuredGrid(const Index numVert_x, const Index numVert_y, const Index numVert_z, const Meta &meta = Meta());

   // get functions for metadata
   Index getNumDivisions(int c) override { return (*d()->numDivisions)[c]; }
   Index getNumDivisions(int c) const override { return m_numDivisions[c]; }
   Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) override;
   Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const override;

   // virtual set functions
   void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) override;

   // GridInterface
   Index getNumVertices() const override;
   std::pair<Vector, Vector> getBounds() const override;
   std::pair<Vector, Vector> cellBounds(Index elem) const override;
   Index findCell(const Vector &point, bool acceptGhost=false) const override;
   bool inside(Index elem, const Vector &point) const override;
   Interpolator getInterpolator(Index elem, const Vector &point, DataBase::Mapping mapping=DataBase::Vertex, InterpolationMode mode=Linear) const override;

   bool hasCelltree() const override;
   Celltree::const_ptr getCelltree() const override;
   bool validateCelltree() const override;

private:
   // mutable pointers to ShmVectors
   mutable Index m_numDivisions[3];
   mutable Index m_ghostLayers[3][2];

   void createCelltree(Index dims[]) const;

   // data object
   V_DATA_BEGIN(StructuredGrid);

   ShmVector<Index> numDivisions; //< number of divisions on each axis (1 more than number of cells)
   ShmVector<Index> ghostLayers[3]; //< number of ghost cell layers in each x, y, z directions

   Data(const Index numVert_x, const Index numVert_y, const Index numVert_z, const std::string & name, const Meta &meta = Meta());
   ~Data();
   static Data *create(const Index numVert_x = 0, const Index numVert_y = 0, const Index numVert_z = 0, const Meta &meta = Meta());

   V_DATA_END(StructuredGrid);
};



BOOST_SERIALIZATION_ASSUME_ABSTRACT(StructuredGrid)

} // namespace vistle

V_OBJECT_DECLARE(vistle::StructuredGrid)

#ifdef VISTLE_IMPL
#include "structuredgrid_impl.h"
#endif

#endif /* STRUCTURED_GRID_H */
