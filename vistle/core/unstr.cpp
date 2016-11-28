#include "unstr.h"
#include <core/assert.h>
#include <algorithm>
#include "archives.h"
#include "cellalgorithm.h"

//#define INTERPOL_DEBUG

namespace vistle {

/* cell types:
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
 */

const int UnstructuredGrid::NumVertices[UnstructuredGrid::POLYHEDRON+1] = {
   0, 2, 3, 4, 4, 5, 6, 8, -1, -1, 1, -1
};
const int UnstructuredGrid::NumFaces[UnstructuredGrid::POLYHEDRON+1] = {
   0, 0, 1, 1, 4, 5, 5, 6, -1, -1, 0, -1
};

const int UnstructuredGrid::FaceSizes[UnstructuredGrid::POLYHEDRON+1][UnstructuredGrid::MaxNumFaces] = {
   // none
   { 0, 0, 0, 0, 0, 0 },
   // bar
   { 0, 0, 0, 0, 0, 0 },
   // triangle
   { 3, 0, 0, 0, 0, 0 },
   // quad
   { 4, 0, 0, 0, 0, 0 },
   // tetrahedron
   { 3, 3, 3, 3, 0, 0 },
   // pyramid
   { 4, 3, 3, 3, 0, 0 },
   // prism
   { 3, 4, 4, 4, 3, 0 },
   // hexahedron
   { 4, 4, 4, 4, 4, 4 },
};

const int UnstructuredGrid::FaceVertices[UnstructuredGrid::POLYHEDRON+1][UnstructuredGrid::MaxNumFaces][UnstructuredGrid::MaxNumVertices] = {
{ // none
},
{ // bar
},
{ // triangle
},
{ // quad
},
{ // tetrahedron
  /*
               3
              /|\
             / | \
            /  |  \
           /   2   \
          /  /   \  \
         / /       \ \
        //           \\
       0---------------1
   */
  { 2, 1, 0 },
  { 0, 1, 3 },
  { 1, 2, 3 },
  { 2, 0, 3 },
},
{ // pyramid
  /*
                 4
             / /  \    \
          0--/------\-------1
         / /          \    /
        //              \ /
       3-----------------2
*/
   { 3, 2, 1, 0 },
   { 0, 1, 4 },
   { 1, 2, 4 },
   { 2, 3, 4 },
   { 3, 0, 4 },
},
{ // prism
  /*
             5
           / | \
         /   |   \
       3 -------- 4
       |     2    |
       |   /   \  |
       | /       \|
       0----------1
  */
   {2, 1, 0},
   {0, 1, 4, 3},
   {1, 2, 5, 4},
   {2, 0, 3, 5},
   {3, 4, 5}
},
{ // hexahedron
  /*
          7 -------- 6
         /|         /|
        / |        / |
       4 -------- 5  |
       |  3-------|--2
       | /        | /
       |/         |/
       0----------1
   */
   { 3, 2, 1, 0 },
   { 4, 5, 6, 7 },
   { 0, 1, 5, 4 },
   { 7, 6, 2, 3 },
   { 1, 2, 6, 5 },
   { 4, 7, 3, 0 },
}
};

UnstructuredGrid::UnstructuredGrid(const Index numElements,
      const Index numCorners,
      const Index numVertices,
      const Meta &meta)
   : UnstructuredGrid::Base(UnstructuredGrid::Data::create(numElements, numCorners, numVertices, meta))
{
    refreshImpl();
}

bool UnstructuredGrid::isEmpty() const {

   return Base::isEmpty();
}

bool UnstructuredGrid::checkImpl() const {

   V_CHECK(d()->tl->check());
   V_CHECK(d()->tl->size() == getNumElements());
   return true;
}

bool UnstructuredGrid::isGhostCell(const Index elem) const {

   if (elem == InvalidIndex)
      return false;
   return tl()[elem] & GHOST_BIT;
}

std::pair<Vector, Vector> UnstructuredGrid::cellBounds(Index elem) const {

   const Index *el = &this->el()[0];
   const Index *cl = &this->cl()[el[elem]];
   const Index nvert = el[elem+1]-el[elem];
   const Scalar *x[3] = { &this->x()[0], &this->y()[0], &this->z()[0] };

   const Scalar smax = std::numeric_limits<Scalar>::max();
   Vector min(smax, smax, smax), max(-smax, -smax, -smax);
   for (Index i=0; i<nvert; ++i) {
       Index v=cl[i];
       for (int c=0; c<3; ++c) {
           min[c] = std::min(min[c], x[c][v]);
           max[c] = std::max(max[c], x[c][v]);
       }
   }
   return std::make_pair(min, max);
}

Index UnstructuredGrid::findCell(const Vector &point, bool acceptGhost) const {

   if (hasCelltree()) {

      vistle::PointVisitationFunctor<Scalar, Index> nodeFunc(point);
      vistle::PointInclusionFunctor<UnstructuredGrid, Scalar, Index> elemFunc(this, point);
      getCelltree()->traverse(nodeFunc, elemFunc);
      if (acceptGhost ||!isGhostCell(elemFunc.cell))
         return elemFunc.cell;
      else
         return InvalidIndex;
   }

   Index size = getNumElements();
   for (Index i=0; i<size; ++i) {
      if (acceptGhost || !isGhostCell(i)) {
         if (inside(i, point))
            return i;
      }
   }
   return InvalidIndex;
}

bool UnstructuredGrid::inside(Index elem, const Vector &point) const {

   const Index *el = &this->el()[0];
   const Index *cl = &this->cl()[el[elem]];
   const Scalar *x = &this->x()[0];
   const Scalar *y = &this->y()[0];
   const Scalar *z = &this->z()[0];

   const auto type(tl()[elem] & ~GHOST_BIT);
   if (type == UnstructuredGrid::POLYHEDRON) {
      const Index nVert = el[elem+1] - el[elem];
      Index startVert = InvalidIndex;
      Vector normal(0,0,0);
      Vector v0(0,0,0), edge1(0,0,0);
      Index faceVert = 0;
      for (Index i=0; i<nVert; ++i) {
         const Index v = cl[i];
         if (v == startVert) {
            startVert = InvalidIndex;
            if (normal.dot(point-v0) > 0)
               return false;
            continue;
         }
         if (startVert == InvalidIndex) {
            startVert = v;
            faceVert = 0;
            normal = Vector(0,0,0);
         } else {
            ++faceVert;
         }
         if (faceVert == 0) {
            v0 = Vector(x[v], y[v], z[v]);
         } else if (faceVert == 1) {
            edge1 = Vector(x[v], y[v], z[v])-v0;
         } else {
            normal += edge1.cross(Vector(x[v], y[v], z[v])-v0);
         }
      }
      return true;
   } else {
      const auto numFaces = NumFaces[type];
      const auto &faces = UnstructuredGrid::FaceVertices[type];
      const auto &sizes = UnstructuredGrid::FaceSizes[type];
      for (int f=0; f<numFaces; ++f) {
         Index first = cl[faces[f][0]];
         Index second = cl[faces[f][1]];
         Vector v0(x[first], y[first], z[first]);
         Vector edge1(x[second], y[second], z[second]);
         edge1 -= v0;
         Vector n(0, 0, 0);
         for (int i=2; i<sizes[f]; ++i) {
            Index other = cl[faces[f][i]];
            Vector edge(x[other], y[other], z[other]);
            edge -= v0;
            n += edge1.cross(edge);
         }

         //std::cerr << "normal: " << n.transpose() << ", v0: " << v0.transpose() << ", rel: " << (point-v0).transpose() << ", dot: " << n.dot(point-v0) << std::endl;

         if (n.dot(point-v0) > 0)
            return false;
      }
      return true;
   }

   return false;
}

namespace {

// cf. http://stackoverflow.com/questions/808441/inverse-bilinear-interpolation
Vector2 bilinearInverse(const Vector &p0, const Vector p[4]) {

    const int iter = 5;
    const Scalar tol = 1e-6;
    Vector2 ss(0.5, 0.5); // initial guess

    const Scalar tol2 = tol*tol;
    for (int k=0; k<iter; ++k) {
       const Scalar s(ss[0]), t(ss[1]);
       Vector res
             = p[0]*(1-s)*(1-t) + p[1]*s*(1-t)
             + p[2]*s*t     + p[3]*(1-s)*t
             - p0;

       //std::cerr << "iter: " << k << ", res: " << res.transpose() << ", weights: " << ss.transpose() << std::endl;

       if (res.squaredNorm() < tol2)
          break;
       const auto Js = -p[0]*(1-t) + p[1]*(1-t) + p[2]*t - p[3]*t;
       const auto Jt = -p[0]*(1-s) - p[1]*s + p[2]*s + p[3]*(1-s);
       Matrix3x2 J;
       J << Js, Jt;
       ss -= (J.transpose()*J).llt().solve(J.transpose()*res);
    }
    return ss;
}

bool insideConvexPolygon(const Vector &point, const Vector *corners, Index nCorners, const Vector &normal) {

   // project into 2D and transform point into origin
   Index max = 0;
   normal.cwiseAbs().maxCoeff(&max);
   int c1=0, c2=1;
   if (max == 0) {
      c1 = 1;
      c2 = 2;
   } else if (max == 1) {
      c1 = 0;
      c2 = 2;
   }
   Vector2 point2;
   point2 << point[c1], point[c2];
   std::vector<Vector2> corners2(nCorners);
   for (Index i=0; i<nCorners; ++i) {
      corners2[i] << corners[i][c1], corners[i][c2];
      corners2[i] -= point2;
   }

   // check whether edges pass the origin at the same side
   Scalar sign(0);
   for (Index i=0; i<nCorners; ++i) {
      Vector2 n = corners2[(i+1)%nCorners] - corners2[i];
      std::swap(n[0],n[1]);
      n[0] *= -1;
      if (i==0) {
          sign = n.dot(corners2[i]);
      } else if (n.dot(corners2[i])*sign < 0) {
#ifdef INTERPOL_DEBUG
         std::cerr << "outside: normal: " << normal.transpose() << ", point: " << point.transpose() << ", p2: " << point2.transpose() << std::endl;
#endif
         return false;
      }
   }

#ifdef INTERPOL_DEBUG
   std::cerr << "inside: normal: " << normal.transpose() << ", point: " << point.transpose() << ", p2: " << point2.transpose() << std::endl;
#endif

   return true;
}

} // anon namespace

GridInterface::Interpolator UnstructuredGrid::getInterpolator(Index elem, const Vector &point, Mapping mapping, InterpolationMode mode) const {

   vassert(inside(elem, point));

#ifdef INTERPOL_DEBUG
   if (!inside(elem, point)) {
      return Interpolator();
   }
#endif

   if (mapping == Element) {
       std::vector<Scalar> weights(1, 1.);
       std::vector<Index> indices(1, elem);

       return Interpolator(weights, indices);
   }

   const auto el = &this->el()[0];
   const auto tl = &this->tl()[0];
   const auto cl = &this->cl()[el[elem]];
   const Scalar *x[3] = { &this->x()[0], &this->y()[0], &this->z()[0] };

   const Index nvert = el[elem+1] - el[elem];
   std::vector<Scalar> weights((mode==Linear || mode==Mean) ? nvert : 1);
   std::vector<Index> indices((mode==Linear || mode==Mean) ? nvert : 1);

   if (mode == Mean) {
      if ((tl[elem] & ~GHOST_BIT) == POLYHEDRON) {
         for (Index i=0; i<nvert; ++i) {
            indices[i] = cl[i];
         }
         std::sort(indices.begin(), indices.end());
         const auto end = std::unique(indices.begin(), indices.end());
         const Index n = end-indices.begin();
         indices.resize(n);
         weights.resize(n);
         Scalar w = Scalar(1)/n;
         for (Index i=0; i<n; ++i)
            weights[i] = w;
      } else {
         const Scalar w = Scalar(1)/nvert;
         for (Index i=0; i<nvert; ++i) {
            indices[i] = cl[i];
            weights[i] = w;
         }
      }
   } else if (mode == Linear) {
      switch(tl[elem] & ~GHOST_BIT) {
         case TETRAHEDRON: {
            vassert(nvert == 4);
            Vector coord[4];
            for (int i=0; i<4; ++i) {
               const Index ind = cl[i];
               indices[i] = ind;
               for (int c=0; c<3; ++c) {
                  coord[i][c] = x[c][ind];
               }
            }
            Matrix3 T;
            T << coord[0]-coord[3], coord[1]-coord[3], coord[2]-coord[3];
            Vector3 w = T.inverse() * (point-coord[3]);
            weights[3] = 1.;
            for (int c=0; c<3; ++c) {
               weights[c] = w[c];
               weights[3] -= w[c];
            }
            break;
         }
         case PYRAMID: {
            vassert(nvert == 5);
            Vector coord[5];
            for (int i=0; i<5; ++i) {
               const Index ind = cl[i];
               indices[i] = ind;
               for (int c=0; c<3; ++c) {
                  coord[i][c] = x[c][ind];
               }
            }
            const Vector first(coord[1] - coord[0]);
            Vector normal(0, 0, 0);
            for (int i=2; i<4; ++i) {
               normal += first.cross(coord[i]-coord[i-1]);
            }
            const Vector top = coord[4] - coord[0];
            const Scalar h = normal.dot(top);
            const Scalar hp = normal.dot(point-coord[0]);
            weights[4] = hp/h;
            const Scalar w = 1-weights[4];
            const Vector p = (point-weights[4]*coord[4])/w;
            const Vector2 ss = bilinearInverse(p, coord);
            weights[0] = (1-ss[0])*(1-ss[1])*w;
            weights[1] = ss[0]*(1-ss[1])*w;
            weights[2] = ss[0]*ss[1]*w;
            weights[3] = (1-ss[0])*ss[1]*w;
            break;
         }
         case PRISM: {
            vassert(nvert == 6);
            Vector coord[8];
            for (int i=0; i<3; ++i) {
               const Index ind1 = cl[i];
               const Index ind2 = cl[i+3];
               indices[i] = ind1;
               indices[i+3] = ind2;
               for (int c=0; c<3; ++c) {
                  coord[i][c] = x[c][ind1];
                  coord[i+4][c] = x[c][ind2];
               }
            }
            // we interpolate in a hexahedron with coinciding corners
            coord[3] = coord[2];
            coord[7] = coord[6];
            const Vector ss = trilinearInverse(point, coord);
            weights[0] = (1-ss[0])*(1-ss[1])*(1-ss[2]);
            weights[1] = ss[0]*(1-ss[1])*(1-ss[2]);
            weights[2] = ss[1]*(1-ss[2]);
            weights[3] = (1-ss[0])*(1-ss[1])*ss[2];
            weights[4] = ss[0]*(1-ss[1])*ss[2];
            weights[5] = ss[1]*ss[2];
            break;
         }
         case HEXAHEDRON: {
            vassert(nvert == 8);
            Vector coord[8];
            for (int i=0; i<8; ++i) {
               const Index ind = cl[i];
               indices[i] = ind;
               for (int c=0; c<3; ++c) {
                  coord[i][c] = x[c][ind];
               }
            }
            const Vector ss = trilinearInverse(point, coord);
            weights[0] = (1-ss[0])*(1-ss[1])*(1-ss[2]);
            weights[1] = ss[0]*(1-ss[1])*(1-ss[2]);
            weights[2] = ss[0]*ss[1]*(1-ss[2]);
            weights[3] = (1-ss[0])*ss[1]*(1-ss[2]);
            weights[4] = (1-ss[0])*(1-ss[1])*ss[2];
            weights[5] = ss[0]*(1-ss[1])*ss[2];
            weights[6] = ss[0]*ss[1]*ss[2];
            weights[7] = (1-ss[0])*ss[1]*ss[2];
            break;
         }
         case POLYHEDRON: {

            /* subdivide n-hedron into n pyramids with tip at the center,
               interpolate within pyramid containing point */

            // polyhedron compute center
            std::vector<Vector> coord(nvert);
            Vector center(0, 0, 0);
            Index startVert = InvalidIndex;
            Index nfaces = 0;
            for (Index i=0; i<nvert; ++i) {
               const Index k = cl[i];
               indices[i] = k;
               for (int c=0; c<3; ++c) {
                  coord[i][c] = x[c][k];
               }
               if (k == startVert) {
                  startVert = InvalidIndex;
                  ++nfaces;
                  continue;
               }
               if (startVert == InvalidIndex) {
                  startVert = k;
               }
               center += coord[i];
            }
            center /= (nvert-nfaces);
#ifdef INTERPOL_DEBUG
            std::cerr << "center: " << center.transpose() << std::endl;
            assert(inside(elem, center));
#endif

            // find face that is hit by ray from polyhedron center through query point
            Index nFaceVert = 0;
            Vector normal, edge1, isect;
            startVert = InvalidIndex;
            Index startIndex = 0;
            Scalar scale = 0;
            bool foundFace = false;
            for (Index i=0; i<nvert; ++i) {
               const Index k = cl[i];
               if (k == startVert) {
                  startVert = InvalidIndex;
                  const Vector dir = point-center;
                  scale = normal.dot(dir) / normal.dot(coord[startIndex]-center);
#ifdef INTERPOL_DEBUG
                  std::cerr << "face: endvert=" << i << ", numvert=" << i-startIndex << ", scale=" << scale << std::endl;
#endif
                  assert(scale <= 1); // otherwise, point is outside of the polyhedron
                  if (scale > 0) {
                     isect = center + dir/scale;
                     std::cerr << "isect: " << isect.transpose() << std::endl;
                     if (insideConvexPolygon(isect, &coord[startIndex], nFaceVert, normal)) {
#ifdef INTERPOL_DEBUG
                        std::cerr << "found face: normal: " << normal.transpose() << ", first: " << coord[startIndex].transpose() << ", dir: " << dir.transpose() << ", isect: " << isect.transpose() << std::endl;
#endif
                        foundFace = true;
                        break;
                     }
                  }
                  continue;
               }
               if (startVert == InvalidIndex) {
                  startVert = k;
                  startIndex = i;
                  nFaceVert = 0;
                  normal = Vector(0, 0, 0);
               } else if (nFaceVert == 1) {
                  edge1 = coord[i]-coord[i-1];
               } else {
                  normal += edge1.cross(coord[i]-coord[i-1]);
               }
               ++nFaceVert;
            }

            if (foundFace) {
               // compute contribution of polyhedron center
               Scalar centerWeight = 1 - scale;
#ifdef INTERPOL_DEBUG
               std::cerr << "center weight: " << centerWeight << ", scale: " << scale << std::endl;
#endif
               Index startVert = InvalidIndex;
               Scalar sum = 0;
               for (Index i=0; i<nvert; ++i) {
                  const Index k = cl[i];
                  if (startVert == InvalidIndex) {
                     startVert = k;
                  } else if (k == startVert) {
                     startVert = InvalidIndex;
                     weights[i] = 0;
                     continue;
                  }
                  Scalar centerDist = (coord[i] - center).norm();
#ifdef INTERPOL_DEBUG
                  //std::cerr << "ind " << i << ", centerDist: " << centerDist << std::endl;
#endif
                  weights[i] = 1/centerDist;
                  sum += weights[i];
               }
#ifdef INTERPOL_DEBUG
               std::cerr << "sum: " << sum << std::endl;
#endif
               for (Index i=0; i<nvert; ++i) {
                  weights[i] *= centerWeight/sum;
               }

               // contribution of hit face,
               // interpolate compatible with simple cells for faces with 3 or 4 vertices
               if (nFaceVert == 3) {
                  Matrix2 T;
                  T << (coord[startIndex+0]-coord[startIndex+2]).block<2,1>(0,0), (coord[startIndex+1]-coord[startIndex+2]).block<2,1>(0,0);
                  Vector2 w = T.inverse() * (isect-coord[startIndex+2]).block<2,1>(0,0);
                  weights[startIndex] += w[0] * (1-centerWeight);
                  weights[startIndex+1] += w[1] * (1-centerWeight);
                  weights[startIndex+2] += (1-w[0]-w[1]) * (1-centerWeight);
               } else if (nFaceVert == 4) {
                  Vector2 ss = bilinearInverse(isect, &coord[startIndex]);
                  weights[startIndex] += (1-ss[0])*(1-ss[1])*(1-centerWeight);
                  weights[startIndex+1] += ss[0]*(1-ss[1])*(1-centerWeight);
                  weights[startIndex+2] += ss[0]*ss[1]*(1-centerWeight);
                  weights[startIndex+3] += (1-ss[0])*ss[1]*(1-centerWeight);
               } else {
                  // compute center of face and subdivide face into triangles
                  Vector3 faceCenter(0, 0, 0);
                  for (Index i=0; i<nFaceVert; ++i) {
                     faceCenter += coord[startIndex+i];
                  }
                  faceCenter /= nFaceVert;

                  Scalar sum = 0;
                  std::vector<Scalar> fweights(nFaceVert);
                  for (Index i=0; i<nFaceVert; ++i) {
                     Scalar centerDist = (coord[i] - faceCenter).norm();
                     fweights[i] = 1/centerDist;
                     sum += fweights[i];
                  }
                  for (Index i=0; i<nFaceVert; ++i) {
                     weights[i+startIndex] += fweights[i]/sum*(1-centerWeight);
                  }
               }
            }
            break;
         }
      }
   } else {
      weights[0] = 1;

      if (mode == First) {
         indices[0] = cl[0];
      } else if(mode == Nearest) {
         Scalar mindist = std::numeric_limits<Scalar>::max();

         for (Index i=0; i<nvert; ++i) {
            const Index k = cl[i];
            const Vector3 vert(x[0][k], x[1][k], x[2][k]);
            const Scalar dist = (point-vert).squaredNorm();
            if (dist < mindist) {
               mindist = dist;
               indices[0] = k;
            }
         }
      }
   }

   return Interpolator(weights, indices);
}

void UnstructuredGrid::refreshImpl() const {
   const Data *d = static_cast<Data *>(m_data);
   m_tl = (d && d->tl.valid()) ? d->tl->data() : nullptr;
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
{
   tl.construct(numElements);
}

UnstructuredGrid::Data * UnstructuredGrid::Data::create(const Index numElements,
                                            const Index numCorners,
                                            const Index numVertices,
                                            const Meta &meta) {

   const std::string name = Shm::the().createObjectId();
   Data *u = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
   publish(u);

   return u;
}

V_OBJECT_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID);
V_OBJECT_CTOR(UnstructuredGrid);

} // namespace vistle
