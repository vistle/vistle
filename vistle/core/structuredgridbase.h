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
   static inline int obtainCellIndex(const int i[3], const int dims[3])
   {
       return (i[0] * dims[1] + i[1]) * dims[2] + i[2];
   }


   // virtual get/set functions for metadata
   virtual Index getNumElements_x() const { return 0; }
   virtual Index getNumElements_y() const { return 0; }
   virtual Index getNumElements_z() const { return 0; }

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
