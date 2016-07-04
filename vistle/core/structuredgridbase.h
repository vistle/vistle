//-------------------------------------------------------------------------
// STRUCTURED GRID OBJECT BASE CLASS H
// *
// * Base class for Structured Grid Objects
//-------------------------------------------------------------------------
#ifndef STRUCTURED_GRID_BASE_H
#define STRUCTURED_GRID_BASE_H

#include "scalar.h"
#include "shm.h"
#include "object.h"
#include "export.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF STRUCTUREDGRIDBASE
//-------------------------------------------------------------------------
class V_COREEXPORT StructuredGridBase : public Object {
   V_OBJECT(StructuredGridBase);

 public:
   typedef Object Base;

   StructuredGridBase(const Meta &meta);

   // static inline method to obtain a cell index from (x,y,z) indices and max grid dimensions
   static inline Index vertexIndex(const Index ix, const Index iy, const Index iz, const Index dims[3])
   {
       return (ix * dims[1] + iy) * dims[2] + iz;
   }
   static inline Index vertexIndex(const Index i[3], const Index dims[3])
   {
       return vertexIndex(i[0], i[1], i[2], dims);
   }

   // virtual get/set functions for metadata
   virtual Index getNumElements() { return (getNumDivisions(0)-1)*(getNumDivisions(1)-1)*(getNumDivisions(2)-1); }
   virtual Index getNumElements() const { return (getNumDivisions(0)-1)*(getNumDivisions(1)-1)*(getNumDivisions(2)-1); }
   virtual Index getNumDivisions(int d) { return 1; }
   virtual Index getNumDivisions(int d) const { return 1; }

   // data object
   V_DATA_BEGIN(StructuredGridBase);

   Data(const Data &o, const std::string &name, Type id);
   ~Data();
   static Data *create(Type id = UNKNOWN, const Meta &meta=Meta());

   V_DATA_END(StructuredGridBase);
};



BOOST_SERIALIZATION_ASSUME_ABSTRACT(StructuredGridBase)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "structuredgridbase_impl.h"
#endif

#endif /* STRUCTURED_GRID_BASE_H */
