#ifndef UNSTRUCTUREDGRID_H
#define UNSTRUCTUREDGRID_H


#include "shm.h"
#include "indexed.h"

namespace vistle {

class V_COREEXPORT UnstructuredGrid: public Indexed {
   V_OBJECT(UnstructuredGrid);

 public:
   typedef Indexed Base;

   // make sure that these types match those from COVISE: src/kernel/do/coDoUnstructuredGrid.h
   enum Type {
      GHOST_BIT = 0x80,

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

   static const int NumVertices[POLYHEDRON+1];
   static const int NumFaces[POLYHEDRON+1];

   UnstructuredGrid(const Index numElements,
         const Index numCorners,
         const Index numVertices,
         const Meta &meta=Meta());

   shm<unsigned char>::array &tl() const { return *(*d()->tl)(); }

   Index findCell(const Vector &point) const;
   bool inside(Index elem, const Vector &point) const;
   std::pair<Vector, Vector> getBounds() const;

   class Interpolator {
      friend class UnstructuredGrid;
      std::vector<Scalar> weights;
      std::vector<Index> indices;
      Interpolator() {}
      Interpolator(std::vector<Scalar> &weights, std::vector<Index> &indices)
#if __cplusplus >= 201103L
      : weights(std::move(weights)), indices(std::move(indices))
#else
      : weights(weights), indices(indices)
#endif
      {}

    public:
      Scalar operator()(const Scalar *field) const {
         Scalar ret(0);
         for (size_t i=0; i<weights.size(); ++i)
            ret += field[indices[i]] * weights[i];
         return ret;
      }

      Vector3 operator()(const Scalar *f0, const Scalar *f1, const Scalar *f2) const {
         Vector3 ret(0, 0, 0);
         for (size_t i=0; i<weights.size(); ++i) {
            const Index ind(indices[i]);
            const Scalar w(weights[i]);
            ret += Vector3(f0[ind], f1[ind], f2[ind]) * w;
         }
         return ret;
      }
   };

   Interpolator getInterpolator(Index elem, const Vector &point) const;
   Interpolator getInterpolator(const Vector &point) const;

   V_DATA_BEGIN(UnstructuredGrid);
      ShmVector<unsigned char>::ptr tl;

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

#ifdef VISTLE_IMPL
#include "unstr_impl.h"
#endif
#endif
