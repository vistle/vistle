//-------------------------------------------------------------------------
// RECTILINEAR GRID CLASS H
// *
// * Rectilinear Grid Container Object
//-------------------------------------------------------------------------
#ifndef RECTILINEAR_GRID_H
#define RECTILINEAR_GRID_H

#include "scalar.h"
#include "shm.h"
#include "structuredgridbase.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF RECTILINEARGRID
//-------------------------------------------------------------------------
class V_COREEXPORT RectilinearGrid : public StructuredGridBase {
   V_OBJECT(RectilinearGrid);

public:
   typedef StructuredGridBase Base;

   // constructor
   RectilinearGrid(const Index NumElements_x, const Index NumElements_y, const Index NumElements_z, const Meta &meta=Meta());

   // get/set functions for metadata
   Index getNumElements_x() const override { return d()->coords_x->size() - 1; }
   Index getNumElements_y() const override { return d()->coords_y->size() - 1; }
   Index getNumElements_z() const override { return d()->coords_z->size() - 1; }

   // get/set functions for shared memory members
   shm<Scalar>::array & coords_x() { return *d()->coords_x; }
   shm<Scalar>::array & coords_y() { return *d()->coords_y; }
   shm<Scalar>::array & coords_z() { return *d()->coords_z; }
   const Scalar * coords_x() const { return m_coords_x; }
   const Scalar * coords_y() const { return m_coords_y; }
   const Scalar * coords_z() const { return m_coords_z; }

private:
   // mutable pointers to ShmVectors
   mutable const Scalar * m_coords_x;
   mutable const Scalar * m_coords_y;
   mutable const Scalar * m_coords_z;

   // data object
   V_DATA_BEGIN(RectilinearGrid);

   ShmVector<Scalar> coords_x; //< coordinates of divisions in x
   ShmVector<Scalar> coords_y; //< coordinates of divisions in y
   ShmVector<Scalar> coords_z; //< coordinates of divisions in z

   Data(const Index NumElements_x, const Index NumElements_y, const Index NumElements_z, const std::string & name, const Meta &meta=Meta());
   ~Data();
   static Data *create(const Index NumElements_x = 0, const Index NumElements_y = 0, const Index NumElements_z = 0, const Meta &meta = Meta());

   V_DATA_END(RectilinearGrid);
};



BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectilinearGrid)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "rectilineargrid_impl.h"
#endif

#endif /* RECTILINEAR_GRID_H */
