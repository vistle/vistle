#ifndef UNSTRUCTUREDGRID_H
#define UNSTRUCTUREDGRID_H


#include "shm.h"
#include "indexed.h"
#include "grid.h"
#include <util/enum.h>

namespace vistle {

class V_COREEXPORT UnstructuredGrid: public Indexed, virtual public GridInterface {
   V_OBJECT(UnstructuredGrid);

 public:
   typedef Indexed Base;

   // make sure that these types match those from COVISE: src/kernel/do/coDoUnstructuredGrid.h
   enum Type {
      GHOST_BIT   = 0x80,
      CONVEX_BIT  = 0x40, //<! cell was checked to be convex
      TYPE_MASK   = 0x3f,

      NONE        =  0,
      BAR         =  1,
      TRIANGLE    =  2,
      QUAD        =  3,
      TETRAHEDRON =  4,
      PYRAMID     =  5,
      PRISM       =  6,
      HEXAHEDRON  =  7,
      POINT       = 10,
      POLYHEDRON  = 11,

      GHOST_TETRAHEDRON = TETRAHEDRON|GHOST_BIT,
      GHOST_PYRAMID     =     PYRAMID|GHOST_BIT,
      GHOST_PRISM       =       PRISM|GHOST_BIT,
      GHOST_HEXAHEDRON  =  HEXAHEDRON|GHOST_BIT,
      GHOST_POLYHEDRON  =  POLYHEDRON|GHOST_BIT
   };

   static const Index MaxNumVertices = 4;
   static const Index MaxNumFaces = 6;

   static const int NumVertices[POLYHEDRON+1];
   static const int NumFaces[POLYHEDRON+1];
   static const int FaceSizes[UnstructuredGrid::POLYHEDRON+1][MaxNumFaces];
   static const int FaceVertices[UnstructuredGrid::POLYHEDRON+1][MaxNumFaces][MaxNumVertices];

   UnstructuredGrid(const Index numElements,
         const Index numCorners,
         const Index numVertices,
         const Meta &meta=Meta());

   shm<unsigned char>::array &tl() { return *d()->tl; }
   const unsigned char *tl() const { return m_tl; }

   bool isConvex(Index elem) const;
   bool isGhostCell(Index elem) const override;
   std::pair<Vector, Vector> cellBounds(Index elem) const override;
   Index findCell(const Vector &point, int flags=NoFlags) const override;
   bool inside(Index elem, const Vector &point) const override;

   Interpolator getInterpolator(Index elem, const Vector &point, Mapping mapping=Vertex, InterpolationMode mode=Linear) const override;

 private:
   mutable const unsigned char *m_tl;

   V_DATA_BEGIN(UnstructuredGrid);
      ShmVector<unsigned char> tl;

      Data(const Index numElements = 0, const Index numCorners = 0,
                    const Index numVertices = 0, const std::string & name = "",
                    const Meta &meta=Meta());
      static Data *create(const Index numElements = 0,
                                    const Index numCorners = 0,
                                    const Index numVertices = 0,
                                    const Meta &meta=Meta());

   V_DATA_END(UnstructuredGrid);

};

} // namespace vistle

V_OBJECT_DECLARE(vistle::UnstructuredGrid)

#ifdef VISTLE_IMPL
#include "unstr_impl.h"
#endif
#endif
