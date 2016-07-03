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
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF STRUCTUREDGRID
//-------------------------------------------------------------------------
class V_COREEXPORT StructuredGrid : public StructuredGridBase {
   V_OBJECT(StructuredGrid);

public:
   typedef StructuredGridBase Base;

   // constructor
   StructuredGrid(const Index numVert_x, const Index numVert_y, const Index numVert_z, const Meta &meta = Meta());

   // get/set functions for metadata
   Index getNumDivisions(int c) override { return (*d()->numDivisions)[c]; }
   Index getNumDivisions(int c) const override { return m_numDivisions[c]; }

   // get/set functions for shared memory members
   shm<Scalar>::array & x(int c=0) { return *d()->x[c]; }
   shm<Scalar>::array & y() { return *d()->x[1]; }
   shm<Scalar>::array & z() { return *d()->x[2]; }
   const Scalar * x(int c=0) const { return m_x[c]; }
   const Scalar * y() const { return m_x[1]; }
   const Scalar * z() const { return m_x[2]; }

private:
   // mutable pointers to ShmVectors
   mutable Index m_numDivisions[3];
   mutable const Scalar *m_x[3];

   // data object
   V_DATA_BEGIN(StructuredGrid);

   ShmVector<Index> numDivisions; //< number of divisions on each axis (1 more than number of cells)
   ShmVector<Scalar> x[3]; //< coordinates of corners (x, y, and z)

   Data(const Index numVert_x, const Index numVert_y, const Index numVert_z, const std::string & name, const Meta &meta = Meta());
   ~Data();
   static Data *create(const Index numVert_x = 0, const Index numVert_y = 0, const Index numVert_z = 0, const Meta &meta = Meta());

   V_DATA_END(StructuredGrid);
};



BOOST_SERIALIZATION_ASSUME_ABSTRACT(StructuredGrid)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "structuredgrid_impl.h"
#endif

#endif /* STRUCTURED_GRID_H */
