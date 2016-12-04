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
#include "grid.h"

namespace vistle {

//-------------------------------------------------------------------------
// DECLARATION OF STRUCTUREDGRIDBASE
//-------------------------------------------------------------------------
class V_COREEXPORT StructuredGridBase: public GridInterface {

public:
   typedef StructuredGridBase Class;
   typedef boost::shared_ptr<Class> ptr;
   typedef boost::shared_ptr<const Class> const_ptr;
   static boost::shared_ptr<const Class> as(boost::shared_ptr<const Object> ptr) { return boost::dynamic_pointer_cast<const Class>(ptr); }
   static boost::shared_ptr<Class> as(boost::shared_ptr<Object> ptr) { return boost::dynamic_pointer_cast<Class>(ptr); }

   enum GhostLayerPosition {
       Bottom,
       Top
   };

   // static inline method to obtain a cell index from (x,y,z) indices and max grid dimensions
   static inline Index vertexIndex(const Index ix, const Index iy, const Index iz, const Index dims[3]) {
       assert(ix < dims[0]);
       assert(iy < dims[1]);
       assert(iz < dims[2]);
       return (ix * dims[1] + iy) * dims[2] + iz;
   }
   static inline Index vertexIndex(const Index i[3], const Index dims[3]) {
       return vertexIndex(i[0], i[1], i[2], dims);
   }
   static inline Index cellIndex(const Index ix, const Index iy, const Index iz, const Index dims[3]) {
       assert(ix < dims[0]-1);
       assert(iy < dims[1]-1);
       assert(iz < dims[2]-1);
       return (ix * (dims[1]-1) + iy) * (dims[2]-1) + iz;
   }
   static inline Index cellIndex(const Index i[3], const Index dims[3]) {
       return cellIndex(i[0], i[1], i[2], dims);
   }
   static inline std::array<Index,3> cellCoordinates(Index el, const Index dims[3]) {
       std::array<Index,3> coords;
       coords[2] = el%(dims[2]-1);
       el /= dims[2]-1;
       coords[1] = el%(dims[1]-1);
       el /= dims[1]-1;
       coords[0] = el;
       return coords;
   }
   static inline std::array<Index,8> cellVertices(Index el, const Index dims[3]) {
       std::array<Index,3> n = cellCoordinates(el, dims);
       std::array<Index,8> cl;
       cl[0] = vertexIndex(n[0], n[1], n[2], dims);
       cl[1] = vertexIndex(n[0]+1, n[1], n[2], dims);
       cl[2] = vertexIndex(n[0]+1, n[1]+1, n[2], dims);
       cl[3] = vertexIndex(n[0], n[1]+1, n[2], dims);
       cl[4] = vertexIndex(n[0], n[1], n[2]+1, dims);
       cl[5] = vertexIndex(n[0]+1, n[1], n[2]+1, dims);
       cl[6] = vertexIndex(n[0]+1, n[1]+1, n[2]+1, dims);
       cl[7] = vertexIndex(n[0], n[1]+1, n[2]+1, dims);
       return cl;
   }

   virtual bool isGhostCell(Index elem) const override;

   // virtual get functions
   virtual Index getNumElements() const override { return (getNumDivisions(0)-1)*(getNumDivisions(1)-1)*(getNumDivisions(2)-1); }
   virtual Index getNumDivisions(int d) { return 1; }           //< get number of vertices in dimension d
   virtual Index getNumDivisions(int d) const { return 1; }     //< get number of vertices in dimension d
   virtual Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) { return 0; }
   virtual Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const { return 0; }

   // virtual set functions
   virtual void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) { return; }
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(StructuredGridBase)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "structuredgridbase_impl.h"
#endif

#endif /* STRUCTURED_GRID_BASE_H */
