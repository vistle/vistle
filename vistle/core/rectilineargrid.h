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
   Index getNumDivisions(int c) override { return d()->coords[c]->size(); }
   Index getNumDivisions(int c) const override { return m_numDivisions[c]; }

   // get/set functions for shared memory members
   shm<Scalar>::array & coords(int c) { return *d()->coords[c]; }
   const Scalar * coords(int c) const { return m_coords[c]; }

private:
   // mutable pointers to ShmVectors
   mutable Index m_numDivisions[3];
   mutable const Scalar *m_coords[3];

   // data object
   V_DATA_BEGIN(RectilinearGrid);

   ShmVector<Scalar> coords[3]; //< coordinates of divisions in x, y, and z

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
