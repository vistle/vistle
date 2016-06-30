//-------------------------------------------------------------------------
// UNIFORM GRID CLASS H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------
#ifndef UNIFORM_GRID_H
#define UNIFORM_GRID_H

#include "scalar.h"
#include "shm.h"
#include "structuredgridbase.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF UNIFORMGRID
//-------------------------------------------------------------------------
class V_COREEXPORT UniformGrid : public StructuredGridBase {
   V_OBJECT(UniformGrid);

public:
   typedef StructuredGridBase Base;

   // constructor
   UniformGrid(const Meta &meta);

   // get/set functions for metadata
   Index getSize_x() const override { return m_size[0]; }
   Index getSize_y() const override { return m_size[1]; }
   Index getSize_z() const override { return m_size[2]; }

   // get/set functions for shared memory members
   shm<Index>::array & size() { return *d()->size; }
   shm<double>::array & min() { return *d()->min; }
   shm<double>::array & max() { return *d()->max; }
   const Index * size() const { return m_size; }
   const double * min() const { return m_min; }
   const double * max() const { return m_max; }

private:
   // mutable pointers to ShmVectors
   mutable const Index * m_size;
   mutable const double * m_min;
   mutable const double * m_max;

   // data object
   V_DATA_BEGIN(UniformGrid);

   // each of the following variables represents a coordinate (by index, in order x, y, z)
   ShmVector<Index> size; //< number of divisions on each axis
   ShmVector<double> min; //< coordinates of minimum grid point
   ShmVector<double> max; //< coordinates of maximum grid point

   Data(const std::string & name, const Meta &meta=Meta());
   ~Data();
   static Data *create(const Meta &meta = Meta());

   V_DATA_END(UniformGrid);
};



BOOST_SERIALIZATION_ASSUME_ABSTRACT(UniformGrid)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "uniformgrid_impl.h"
#endif

#endif /* UNIFORM_GRID_H */
