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
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF UNIFORMGRID
//-------------------------------------------------------------------------
class V_COREEXPORT UniformGrid : public Object, public StructuredGridBase {
   V_OBJECT(UniformGrid);

public:
   typedef Object Base;

   // constructor
   UniformGrid(Index xdim, Index ydim, Index zdim, const Meta &meta=Meta());

   // get/set functions
   Index getNumDivisions(int c) override { return (*d()->numDivisions)[c]; }
   Index getNumDivisions(int c) const override { return m_numDivisions[c]; }
   Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) override;
   Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const override;

   // virtual set functions
   void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) override;

   // get/set functions for shared memory members
   shm<Scalar>::array & min() { return *d()->min; }
   shm<Scalar>::array & max() { return *d()->max; }
   const Scalar * min() const { return m_min; }
   const Scalar * max() const { return m_max; }

   // GeometryInterface
   std::pair<Vector, Vector> getBounds() const override;

   // GridInterface
   Index getNumVertices() const override;
   std::pair<Vector, Vector> cellBounds(Index elem) const override;
   Index findCell(const Vector &point, Index hint=InvalidIndex, int flags=NoFlags) const override;
   bool inside(Index elem, const Vector &point) const override;
   Interpolator getInterpolator(Index elem, const Vector &point, DataBase::Mapping mapping=DataBase::Vertex, InterpolationMode mode=Linear) const override;
   Scalar exitDistance(Index elem, const Vector &point, const Vector &dir) const override;
   Vector getVertex(Index v) const override;

private:
   // mutable pointers to ShmVectors
   mutable Index m_numDivisions[3];
   mutable Scalar m_min[3];
   mutable Scalar m_max[3];
   mutable Scalar m_dist[3];
   mutable Index m_ghostLayers[3][2];

   // data object
   V_DATA_BEGIN(UniformGrid);

   // each of the following variables represents a coordinate (by index, in order x, y, z)
   ShmVector<Index> numDivisions; //< number of divisions on each axis (1 more than number of cells)
   ShmVector<Scalar> min; //< coordinates of minimum grid point
   ShmVector<Scalar> max; //< coordinates of maximum grid point
   ShmVector<Index> ghostLayers[3]; //< number of ghost cell layers in each x, y, z directions

   Data(const std::string & name, Index xdim, Index ydim, Index zdim, const Meta &meta=Meta());
   ~Data();
   static Data *create(Index xdim=0, Index ydim=0, Index zdim=0, const Meta &meta = Meta());

   V_DATA_END(UniformGrid);
};



BOOST_SERIALIZATION_ASSUME_ABSTRACT(UniformGrid)

} // namespace vistle

V_OBJECT_DECLARE(vistle::UniformGrid)
#endif /* UNIFORM_GRID_H */

#ifdef VISTLE_IMPL
#include "uniformgrid_impl.h"
#endif
