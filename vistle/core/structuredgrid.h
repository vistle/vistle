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
   Index getNumElements_x() const override { return m_numElements[0]; }
   Index getNumElements_y() const override { return m_numElements[1]; }
   Index getNumElements_z() const override { return m_numElements[2]; }

   // get/set functions for shared memory members
   shm<Index>::array & numElements() { return *d()->numElements; }
   shm<double>::array & coords_x() { return *d()->coords_x; }
   shm<double>::array & coords_y() { return *d()->coords_y; }
   shm<double>::array & coords_z() { return *d()->coords_z; }
   const Index * numElements() const { return m_numElements; }
   const double * coords_x() const { return m_coords_x; }
   const double * coords_y() const { return m_coords_y; }
   const double * coords_z() const { return m_coords_z; }

private:
   // mutable pointers to ShmVectors
   mutable const Index * m_numElements;
   mutable const double * m_coords_x;
   mutable const double * m_coords_y;
   mutable const double * m_coords_z;

   // data object
   V_DATA_BEGIN(StructuredGrid);

   ShmVector<Index> numElements; //< number of divisions on each axis (by index, in order x, y, z)
   ShmVector<double> coords_x; //< coordinates of corners (x)
   ShmVector<double> coords_y; //< coordinates of corners (y)
   ShmVector<double> coords_z; //< coordinates of corners (z)

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
