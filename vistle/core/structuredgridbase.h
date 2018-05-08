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
   typedef std::shared_ptr<Class> ptr;
   typedef std::shared_ptr<const Class> const_ptr;
   static std::shared_ptr<const Class> as(std::shared_ptr<const Object> ptr) { return std::dynamic_pointer_cast<const Class>(ptr); }
   static std::shared_ptr<Class> as(std::shared_ptr<Object> ptr) { return std::dynamic_pointer_cast<Class>(ptr); }

   enum GhostLayerPosition {
       Bottom,
       Top
   };
   static constexpr std::array<Index,8> HexahedronIndices[3] = {{{0,1,1,0,0,1,1,0}}, {{0,0,1,1,0,0,1,1}}, {{0,0,0,0,1,1,1,1}}};

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
   static inline std::array<Index,3> vertexCoordinates(Index v, const Index dims[3]) {
       std::array<Index,3> coords;
       coords[2] = v%dims[2];
       v /= dims[2];
       coords[1] = v%dims[1];
       v /= dims[1];
       coords[0] = v;
       assert(coords[0] < dims[0]);
       assert(coords[1] < dims[1]);
       assert(coords[2] < dims[2]);
       return coords;
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
       assert(coords[0] < dims[0]-1);
       assert(coords[1] < dims[1]-1);
       assert(coords[2] < dims[2]-1);
       return coords;
   }
   static inline std::array<Index,8> cellVertices(Index el, const Index dims[3]) {
       auto &H = HexahedronIndices;
       std::array<Index,3> n = cellCoordinates(el, dims);
       std::array<Index,8> cl;
       for (int i=0; i<8; ++i) {
           cl[i] = vertexIndex(n[0]+H[0][i], n[1]+H[1][i], n[2]+H[2][i], dims);
       }
       return cl;
   }

   virtual bool isGhostCell(Index elem) const override;

   // virtual get functions
   virtual Index getNumElements() const override { return (getNumDivisions(0)-1)*(getNumDivisions(1)-1)*(getNumDivisions(2)-1); }
   virtual Index getNumDivisions(int d) { return 1; }           //< get number of vertices in dimension d
   virtual Index getNumDivisions(int d) const { return 1; }     //< get number of vertices in dimension d
   virtual Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) { return 0; }
   virtual Index getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const { return 0; }
   virtual Vector getVertex(Index v) const = 0;

   // virtual set functions
   virtual void setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value) { return; }

   virtual Scalar cellDiameter(Index elem) const override;
   virtual Vector cellCenter(Index elem) const override;
   virtual std::vector<Index> getNeighborElements(Index elem) const override;
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(StructuredGridBase)

} // namespace vistle
#endif /* STRUCTURED_GRID_BASE_H */

#ifdef VISTLE_IMPL
#include "structuredgridbase_impl.h"
#endif
