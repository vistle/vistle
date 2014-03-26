#include "unstr.h"

namespace vistle {

const int UnstructuredGrid::NumVertices[UnstructuredGrid::POLYHEDRON+1] = {
   0, 2, 3, 4, 4, 5, 6, 8, -1, -1, 1, -1
};

UnstructuredGrid::UnstructuredGrid(const Index numElements,
      const Index numCorners,
      const Index numVertices,
      const Meta &meta)
   : UnstructuredGrid::Base(UnstructuredGrid::Data::create(numElements, numCorners, numVertices, meta))
{
}

bool UnstructuredGrid::isEmpty() const {

   return Base::isEmpty();
}

bool UnstructuredGrid::checkImpl() const {

   V_CHECK(tl().size() == getNumElements());
   return true;
}

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
      for (int i=0; i<3; ++i) {
         if (min[i] > m_point[i])
            return false;
         if (max[i] < m_point[i])
            return false;
      }
      return true;
   }

   Order operator()(const typename Celltree::Node &node, Scalar *min, Scalar *max) {

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
class PointInclusionFunctor: public Celltree<Scalar, Index>::LeafFunctor {

 public:
   PointInclusionFunctor(UnstructuredGrid::const_ptr grid, const Vector &point)
      : m_grid(grid)
      , m_point(point)
      , cell(-1)
   {
   }

   bool operator()(Index elem) {
      if (m_grid->inside(elem, m_point)) {
         return false; // stop traversal
      }
      return true;
   }
   UnstructuredGrid::const_ptr m_grid;
   Vector m_point;
   Index cell;

};


Index UnstructuredGrid::findCell(const Vector &point) const {

   if (hasCelltree()) {

      PointVisitationFunctor<Scalar, Index> nodeFunc(point);
      PointInclusionFunctor<Scalar, Index> elemFunc(UnstructuredGrid::const_ptr(this), point);
      getCelltree()->traverse(nodeFunc, elemFunc);
      return elemFunc.cell;
   }

   Index size = getNumElements();
   Index *elems = el().data();
   for (Index i=0; i<size; ++i) {
      Index e = elems[i];
      if (inside(e, point))
         return e;
   }
   return -1;
}

bool UnstructuredGrid::inside(Index elem, const Vector &point) const {

   const Index *cl = &this->cl()[this->el()[elem]];
   const Scalar *x = this->x().data();
   const Scalar *y = this->y().data();
   const Scalar *z = this->z().data();

   switch (tl().at(elem)) {
      case UnstructuredGrid::HEXAHEDRON:
         int faces[6][4] = {
            { 3, 2, 1, 0 },
            { 4, 5, 6, 7 },
            { 4, 5, 1, 0 },
            { 3, 2, 6, 7 },
            { 5, 6, 2, 1 },
            { 0, 3, 7, 4 },
         };

         for (int f=0; f<6; ++f) {
            Index first = cl[faces[f][0]];
            Index second = cl[faces[f][1]];
            Vector v0(x[first], y[first], z[first]);
            Vector edge1(x[second], y[second], z[second]);
            edge1 -= v0;
            Vector n;
            for (int i=2; i<4; ++i) {
               Index other = cl[faces[f][i]];
               Vector edge(x[other], y[other], z[other]);
               edge -= v0;
               n += edge1.cross(edge);
            }

            if (n.dot(point-v0) > 0)
               return false;
         }
         return true;
         break;
   }

   return false;
}

UnstructuredGrid::Data::Data(const UnstructuredGrid::Data &o, const std::string &n)
: UnstructuredGrid::Base::Data(o, n)
, tl(o.tl)
{
}

UnstructuredGrid::Data::Data(const Index numElements,
                                   const Index numCorners,
                                   const Index numVertices,
                                   const std::string & name,
                                   const Meta &meta)
   : UnstructuredGrid::Base::Data(numElements, numCorners, numVertices,
         Object::UNSTRUCTUREDGRID, name, meta)
   , tl(new ShmVector<unsigned char>(numElements))
{
}

UnstructuredGrid::Data * UnstructuredGrid::Data::create(const Index numElements,
                                            const Index numCorners,
                                            const Index numVertices,
                                            const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *u = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
   publish(u);

   return u;
}

V_OBJECT_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID);

} // namespace vistle
