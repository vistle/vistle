#ifndef UNSTRUCTUREDGRID_H
#define UNSTRUCTUREDGRID_H


#include "shm.h"
#include "indexed.h"
#include <util/enum.h>

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

   void refresh() const override;

   shm<unsigned char>::array &tl() { return *(*d()->tl)(); }
   const unsigned char *tl() const { return m_tl; }

   bool isGhostCell(Index elem) const;
   Index findCell(const Vector &point, bool acceptGhost=false) const;
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

   DEFINE_ENUM_WITH_STRING_CONVERSIONS(InterpolationMode, (First) // value of first vertex
         (Mean) // mean value of all vertices
         (Nearest) // value of nearest vertex
         (Linear) // barycentric/multilinear interpolation
         );

   Interpolator getInterpolator(Index elem, const Vector &point, InterpolationMode mode=Linear) const;
   Interpolator getInterpolator(const Vector &point, InterpolationMode mode=Linear) const;

 private:
   void refreshImpl() const;
   mutable const unsigned char *m_tl;

   V_DATA_BEGIN(UnstructuredGrid);
      ShmVector<unsigned char>::ptr tl;

      Data(const Index numElements = 0, const Index numCorners = 0,
                    const Index numVertices = 0, const std::string & name = "",
                    const Meta &meta=Meta());
      static Data *create(const std::string &name, const Index numElements = 0,
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
