#ifndef VISTLE_CELLINTERPOLATION_H
#define VISTLE_CELLINTERPOLATION_H

#include "export.h"
#include "vector.h"
#include "celltree.h"
#include "grid.h"

namespace vistle {

V_COREEXPORT Vector trilinearInverse(const Vector &p0, const Vector p[8]);

template<typename Scalar, typename Index>
class PointVisitationFunctor: public Celltree<Scalar, Index>::VisitFunctor {
   typedef vistle::Celltree<Scalar, Index> Celltree;
   typedef typename Celltree::VisitFunctor VisitFunctor;
   typedef typename VisitFunctor::Order Order;
 public:
   PointVisitationFunctor(const Vector &point)
   : m_point(point) {
   }

   bool checkBounds(const Scalar *min, const Scalar *max) {
#ifdef CT_DEBUG
      std::cerr << "checkBounds: min: "
         << min[0] << " " << min[1] << " " << min[2]
         << ", max: " << max[0] << " " << max[1] << " " << max[2] << std::endl;
#endif
      for (int i=0; i<3; ++i) {
         if (min[i] > m_point[i])
            return false;
         if (max[i] < m_point[i])
            return false;
      }
      return true;
   }

   Order operator()(const typename Celltree::Node &node) {

#ifdef CT_DEBUG
      std::cerr << "visit subtree: Lmax: " << node.Lmax << ", Rmin: " << node.Rmin << std::endl;
#endif

      const Scalar c = m_point[node.dim];
      if (c > node.Lmax && c < node.Rmin)
         return VisitFunctor::None;
      if (c < node.Rmin) {
         return VisitFunctor::Left;
      }
      if (c > node.Lmax) {
         return VisitFunctor::Right;
      }

      const Scalar mean = Scalar(0.5) * (node.Lmax + node.Rmin);
      if (c < mean) {
         return VisitFunctor::LeftRight;
      } else {
         return VisitFunctor::RightLeft;
      }
   }

 private:
   Vector m_point;
};

template<typename Scalar, typename Index>
struct CellBoundsFunctor: public Celltree<Scalar, Index>::CellBoundsFunctor {

   CellBoundsFunctor(const GridInterface *grid)
      : m_grid(grid)
      {}

   bool operator()(Index elem, Vector *min, Vector *max) const {

      auto b = m_grid->cellBounds(elem);
      *min = b.first;
      *max = b.second;
      return true;
   }

   const GridInterface *m_grid;
};


template<class Grid, typename Scalar, typename Index>
class PointInclusionFunctor: public Celltree<Scalar, Index>::LeafFunctor {

 public:
   PointInclusionFunctor(const Grid *grid, const Vector &point, bool acceptGhost=false)
      : m_grid(grid)
      , m_point(point)
      , m_acceptGhost(acceptGhost)
      , cell(InvalidIndex)
   {
   }

   bool operator()(Index elem) {
#ifdef CT_DEBUG
      std::cerr << "PointInclusionFunctor: checking cell: " << elem << std::endl;
#endif
      if (m_acceptGhost || !m_grid->isGhostCell(elem)) {
          if (m_grid->inside(elem, m_point)) {
#ifdef CT_DEBUG
              std::cerr << "PointInclusionFunctor: found cell: " << elem << std::endl;
#endif
              cell = elem;
              return false; // stop traversal
          }
      }
      return true;
   }
   const Grid *m_grid;
   Vector m_point;
   bool m_acceptGhost;
   Index cell;

};

}
#endif
