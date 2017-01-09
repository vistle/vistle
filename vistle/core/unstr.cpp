#include "unstr.h"
#include <core/assert.h>
#include <algorithm>
#include "archives.h"
#include "cellalgorithm.h"
#include <set>

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

bool UnstructuredGrid::isConvex(const Index elem) const {
   if (elem == InvalidIndex)
      return false;
   return tl()[elem] & CONVEX_BIT || (tl()[elem]&TYPE_MASK) == TETRAHEDRON;
}

bool UnstructuredGrid::isGhostCell(const Index elem) const {

   if (elem == InvalidIndex)
      return false;
   return tl()[elem] & GHOST_BIT;
}

bool UnstructuredGrid::checkConvexity() {

    const Index nelem = getNumElements();
    auto tl = this->tl().data();
    const auto cl = this->cl().data();
    const auto el = this->el().data();
    const auto x = this->x().data();
    const auto y = this->y().data();
    const auto z = this->z().data();

    bool convex = true;
    for (Index elem=0; elem<nelem; ++elem) {
        switch (tl[elem]&TYPE_MASK) {
        case NONE:
        case BAR:
        case TRIANGLE:
        case POINT:
            tl[elem] |= CONVEX_BIT;
            break;
        case QUAD:
            convex = false;
            break;
        case TETRAHEDRON:
            tl[elem] |= CONVEX_BIT;
            break;
        case PRISM:
        case PYRAMID:
        case HEXAHEDRON: {
            bool conv = true;
            const Index begin = el[elem], end = el[elem+1];
            for (Index idx = begin; idx<end; ++idx) {
                const Index v = cl[idx];
                const Vector p(x[v], y[v], z[v]);
                if (!insideConvex(elem, p)) {
                    conv = false;
                    break;
                }
            }
            if (conv) {
                tl[elem] |= CONVEX_BIT;
            } else {
                convex = false;
            }
            break;
        }
        case POLYHEDRON: {
            bool conv = true;
            std::vector<Index> vert = cellVertices(elem);
            for (Index v: vert) {
                const Vector p(x[v], y[v], z[v]);
                if (!insideConvex(elem, p)) {
                    conv = false;
                    break;
                }
            }
            if (conv) {
                tl[elem] |= CONVEX_BIT;
            } else {
                convex = false;
            }
            break;
        }
        default:
            std::cerr << "invalid element type " << (tl[elem]&TYPE_MASK) << std::endl;
            convex = false;
            break;
        }
    }

    return convex;
}

std::pair<Vector, Vector> UnstructuredGrid::cellBounds(Index elem) const {

   return elementBounds(elem);
}

Index UnstructuredGrid::findCell(const Vector &point, int flags) const {

   const bool acceptGhost = flags&AcceptGhost;
   const bool useCelltree = (flags&ForceCelltree) || (hasCelltree() && !(flags&NoCelltree));

   if (useCelltree) {

      vistle::PointVisitationFunctor<Scalar, Index> nodeFunc(point);
      vistle::PointInclusionFunctor<UnstructuredGrid, Scalar, Index> elemFunc(this, point, acceptGhost);
      getCelltree()->traverse(nodeFunc, elemFunc);
      return elemFunc.cell;
   }

   Index size = getNumElements();
   for (Index i=0; i<size; ++i) {
      if (acceptGhost || !isGhostCell(i)) {
         if (inside(i, point)) {
            return i;
         }
      }
   }
   return InvalidIndex;
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

} // anon namespace

Scalar UnstructuredGrid::cellDiameter(Index elem) const {
    // compute distance of some vertices which are distant to each other
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];
    auto verts = cellVertices(elem);
    if (verts.empty())
        return 0;
    Index v0 = verts[0];
    Vector p(x[v0], y[v0], z[v0]);
    Vector farAway(p);
    Scalar dist2(0);
    for (Index v: verts) {
        Vector q(x[v], y[v], z[v]);
        Scalar d = (p-q).squaredNorm();
        if (d > dist2) {
			farAway = q;
            dist2 = d;
        }
    }
    for (Index v: verts) {
        Vector q(x[v], y[v], z[v]);
        Scalar d = (farAway -q).squaredNorm();
        if (d > dist2) {
            dist2 = d;
        }
    }
    return sqrt(dist2);
}

std::vector<Index> UnstructuredGrid::getNeighborElements(Index elem) const {

    return getNeighborFinder().getNeighborElements(elem);
}


bool UnstructuredGrid::insideConvex(Index elem, const Vector &point) const {

   //const Scalar Tolerance = 1e-3*cellDiameter(elem); // too slow: halves particle tracing speed
   const Scalar Tolerance = 1e-5;

   const Index *el = &this->el()[0];
   const Index *cl = &this->cl()[el[elem]];
   const Scalar *x = &this->x()[0];
   const Scalar *y = &this->y()[0];
   const Scalar *z = &this->z()[0];

   const auto type(tl()[elem] & TYPE_MASK);
   if (type == UnstructuredGrid::POLYHEDRON) {
      const Index begin=el[elem], end=el[elem+1];
      const Index nvert = end-begin;
      for (Index i=0; i<nvert; i+=cl[i]+1) {
          auto nc = faceNormalAndCenter(cl[i], &cl[i+1], x, y, z);
          auto normal = nc.first;
          auto center = nc.second;
          if (normal.dot(point-center) > Tolerance)
              return false;
      }
      return true;
   } else {
      const auto numFaces = NumFaces[type];
      for (int f=0; f<numFaces; ++f) {
         auto nc = faceNormalAndCenter(type, f, cl, x, y, z);
         auto normal = nc.first;
         auto center = nc.second;

         //std::cerr << "normal: " << n.transpose() << ", v0: " << v0.transpose() << ", rel: " << (point-v0).transpose() << ", dot: " << n.dot(point-v0) << std::endl;

         if (normal.dot(point-center) > Tolerance)
            return false;
      }
      return true;
   }

   return false;
}

Scalar UnstructuredGrid::exitDistance(Index elem, const Vector &point, const Vector &dir) const {

    const Index *el = &this->el()[0];
    const Index begin=el[elem], end=el[elem+1];
    const Index *cl = &this->cl()[begin];
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];

    const Vector raydir(dir.normalized());

    Scalar exitDist = -1;
    const auto type(tl()[elem] & TYPE_MASK);
    if (type == UnstructuredGrid::POLYHEDRON) {
        const Index nvert = end-begin;
        for (Index i=0; i<nvert; i+=cl[i]+1) {
            const Index nCorners = cl[i];
            auto nc = faceNormalAndCenter(cl[i], &cl[i+1], x, y, z);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(raydir);
            if (std::abs(cosa) <= 1e-7) {
                continue;
            }
            const Scalar t = normal.dot(center-point)/cosa;
            if (t < 0) {
                continue;
            }
            std::vector<Vector> corners(nCorners);
            for (int k=0; k<nCorners; ++k) {
                const Index v = cl[i+k+1];
                corners[k] = Vector(x[v], y[v], z[v]);
            }
            const auto isect = point + t*raydir;
            if (insidePolygon(isect, corners.data(), nCorners, normal)) {
                if (exitDist<0 || t<exitDist)
                    exitDist = t;
            }
        }
    } else {
        const auto numFaces = NumFaces[type];
        Vector corners[4];
        for (int f=0; f<numFaces; ++f) {
            auto nc = faceNormalAndCenter(type, f, cl, x, y, z);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(raydir);
            if (std::abs(cosa) <= 1e-7) {
                continue;
            }
            const Scalar t = normal.dot(center-point)/cosa;
            if (t < 0) {
                continue;
            }
            const Index nCorners = FaceSizes[type][f];
            for (int i=0; i<nCorners; ++i) {
                const Index v = cl[FaceVertices[type][f][i]];
                corners[i] = Vector(x[v], y[v], z[v]);
            }
            const auto isect = point + t*raydir;
            if (insidePolygon(isect, corners, nCorners, normal)) {
                if (exitDist<0 || t<exitDist)
                    exitDist = t;
            }
        }
    }
    return exitDist;
}

bool UnstructuredGrid::inside(Index elem, const Vector &point) const {

   if(elem == InvalidIndex)
       return false;

   return insideConvex(elem, point);
#if 0
    if (isConvex(elem))
        return insideConvex(elem, point);
#endif

    const Vector raydir = Vector(1,1,1).normalized();
    const auto type = tl()[elem] & TYPE_MASK;
    switch (type) {
    case TETRAHEDRON:
        return insideConvex(elem, point);
        assert("already handled in insideConvex"==0);
        break;
    case PYRAMID:
    case PRISM:
    case HEXAHEDRON: {
        const Index begin = el()[elem];
        const Index *cl = &this->cl()[begin];
        const Scalar *x = &this->x()[0];
        const Scalar *y = &this->y()[0];
        const Scalar *z = &this->z()[0];
        int nisect = 0;
        const auto numFaces = NumFaces[type];
        Vector corners[4];
        for (int f=0; f<numFaces; ++f) {
            auto nc = faceNormalAndCenter(type, f, cl, x, y, z);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(raydir);
            if (std::abs(cosa) <= 1e-7) {
                continue;
            }
            const Scalar t = normal.dot(center-point)/cosa;
            if (t < 0 || !std::isfinite(t)) {
                continue;
            }
            const Index nCorners = FaceSizes[type][f];
            for (int i=0; i<nCorners; ++i) {
                const Index v = cl[FaceVertices[type][f][i]];
                corners[i] = Vector(x[v], y[v], z[v]);
            }
            const Vector isect = point + t*raydir;
            //assert(std::abs(normal.dot(isect - center)) < 1e-3);
            if (insidePolygon(isect, corners, nCorners, normal)) {
                ++nisect;
            }
        }
        return (nisect%2) == 1;
        break;
        }
    case POLYHEDRON: {
        const Index begin=el()[elem], end=el()[elem+1];
        const Index *cl = &this->cl()[0];
        const Scalar *x = &this->x()[0];
        const Scalar *y = &this->y()[0];
        const Scalar *z = &this->z()[0];

        int nisect = 0;
        for (Index i=begin; i<end; i+=cl[i]+1) {
            const Index nCorners = cl[i];

            auto nc = faceNormalAndCenter(nCorners, &cl[i+1], x, y, z);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(raydir);
            if (std::abs(cosa) <= 1e-7) {
                continue;
            }
            const Scalar t = normal.dot(center-point)/cosa;
            if (t < 0 || !std::isfinite(t)) {
                continue;
            }
            std::vector<Vector> corners(nCorners);
            for (int k=0; k<nCorners; ++k) {
                const Index v = cl[i+k+1];
                corners[k] = Vector(x[v], y[v], z[v]);
            }
            const Vector isect = point + t*raydir;
            //assert(std::abs(normal.dot(isect - center)) < 1e-3);
            if (insidePolygon(isect, corners.data(), nCorners, normal)) {
                ++nisect;
            }
        }

        return (nisect%2) == 1;
        break;
    }
    default:
        std::cerr << "UnstructuredGrid::inside(" << elem << "): unhandled cell type " << type << std::endl;
    }

    return insideConvex(elem, point);
}

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
      if ((tl[elem] & TYPE_MASK) == POLYHEDRON) {
         indices = cellVertices(elem);
         const Index n = indices.size();
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
      switch(tl[elem] & TYPE_MASK) {
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
            std::vector<Index> verts = cellVertices(elem);
            Vector center(0,0,0);
            for (auto v: verts) {
                Vector c(x[0][v], x[1][v], x[2][v]);
                center += c;
            }
            center /= verts.size();
#ifdef INTERPOL_DEBUG
            std::cerr << "center: " << center.transpose() << std::endl;
            assert(inside(elem, center));
#endif

            std::vector<Vector> coord(nvert);
            Index nfaces = 0;
            Index n=0;
            for (Index i=0; i<nvert; i+=cl[i]+1) {
                const Index N = cl[i];
                ++nfaces;
                for (Index k=i+1; k<i+N+1; ++k) {
                    indices[n] = cl[k];
                    for (int c=0; c<3; ++c) {
                        coord[n][c] = x[c][cl[k]];
                    }
                    ++n;
                }
            }
            Index ncoord = n;
            coord.resize(ncoord);
            weights.resize(ncoord);

            // find face that is hit by ray from polyhedron center through query point
            Scalar scale = 0;
            Vector isect;
            bool foundFace = false;
            n = 0;
            Index nFaceVert = 0;
            Vector faceCenter(0,0,0);
            for (Index i=0; i<nvert; i+=cl[i]+1) {
               nFaceVert = cl[i];
               auto nc = faceNormalAndCenter(nFaceVert, &cl[i+1], x[0], x[1], x[2]);
               const Vector dir = point-center;
               Vector normal = nc.first;
               faceCenter = nc.second;
               scale = normal.dot(dir) / normal.dot(faceCenter-center);
#ifdef INTERPOL_DEBUG
               std::cerr << "face: normal=" << normal.transpose() << ", center=" << faceCenter.transpose() << ", endvert=" << i << ", numvert=" << nFaceVert << ", scale=" << scale << std::endl;
#endif
               assert(scale <= 1.01); // otherwise, point is outside of the polyhedron
               if (scale >= 0) {
                   scale = std::min(Scalar(1), scale);
                   if (scale > 0.001) {
                       isect = center + dir/scale;
#ifdef INTERPOL_DEBUG
                       std::cerr << "isect: " << isect.transpose() << std::endl;
#endif
                       if (insideConvexPolygon(isect, &coord[n], nFaceVert, normal)) {
#ifdef INTERPOL_DEBUG
                           std::cerr << "found face: normal: " << normal.transpose() << ", first: " << coord[n].transpose() << ", dir: " << dir.transpose() << ", isect: " << isect.transpose() << std::endl;
                           assert(insidePolygon(isect, &coord[n], nFaceVert, normal));
#endif
                           foundFace = true;
                           break;
                       } else {
#ifdef INTERPOL_DEBUG
                           assert(!insidePolygon(isect, &coord[n], nFaceVert, normal));
#endif
                       }
                   } else {
                       scale = 0;
                       break;
                   }
               }
               n += nFaceVert;
            }
            const Index startIndex = n;

            // compute contribution of polyhedron center
            Scalar centerWeight = 1 - scale;
#ifdef INTERPOL_DEBUG
            std::cerr << "center weight: " << centerWeight << ", scale: " << scale << std::endl;
#endif
            Scalar sum = 0;
            if (centerWeight >= 0.0001) {
                std::set<Index> usedIndices;
                for (Index i=0; i<ncoord; ++i) {
                    if (!usedIndices.insert(indices[i]).second) {
                        weights[i] = 0;
                        continue;
                    }
                    Scalar centerDist = (coord[i] - center).norm();
#ifdef INTERPOL_DEBUG
                    //std::cerr << "ind " << i << ", centerDist: " << centerDist << std::endl;
#endif
                    if (std::abs(centerDist) > 0) {
                        weights[i] = 1/centerDist;
                        sum += weights[i];
                    } else {
                        for (int j=0; j<ncoord; ++j) {
                            weights[j] = 0;

                        }
                        weights[i] = 1;
                        sum = weights[i];
                        break;
                    }
                }
#ifdef INTERPOL_DEBUG
                std::cerr << "sum: " << sum << std::endl;
#endif
            }

            if (sum > 0) {
                for (Index i=0; i<ncoord; ++i) {
                    weights[i] *= centerWeight/sum;
                }
            } else {
                for (Index i=0; i<ncoord; ++i) {
                    weights[i] = 0;
                }
            }

            if (foundFace) {
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
               } else if (nFaceVert > 0) {
                  // subdivide face into triangles around faceCenter
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

std::pair<Vector, Vector> UnstructuredGrid::elementBounds(Index elem) const {

    if ((tl()[elem]&UnstructuredGrid::TYPE_MASK) != UnstructuredGrid::POLYHEDRON) {
        return Base::elementBounds(elem);
    }

    const Index *el = &this->el()[0];
    const Index *cl = &this->cl()[0];
    const Scalar *x[3] = { &this->x()[0], &this->y()[0], &this->z()[0] };
    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector min(smax, smax, smax), max(-smax, -smax, -smax);
    const Index begin = el[elem], end = el[elem+1];
    for (Index j=begin; j<end; j+=cl[j]+1) {
        Index nvert = cl[j];
        for (Index k=j+1; k<j+nvert+1; ++k) {
            Index v=cl[k];
            for (int c=0; c<3; ++c) {
                min[c] = std::min(min[c], x[c][v]);
                max[c] = std::max(max[c], x[c][v]);
            }
        }
    }
    return std::make_pair(min, max);
}

std::vector<Index> UnstructuredGrid::cellVertices(Index elem) const {
    if ((tl()[elem]&UnstructuredGrid::TYPE_MASK) != UnstructuredGrid::POLYHEDRON) {
        return Base::cellVertices(elem);
    }

    const Index *el = &this->el()[0];
    const Index *cl = &this->cl()[0];
    const Index begin = el[elem], end = el[elem+1];
    std::vector<Index> verts;
    verts.reserve(end-begin);
    for (Index j=begin; j<end; j+= cl[j]+1) {
        Index nvert = cl[j];
        for (Index k=j+1; k<j+nvert+1; ++k) {
            verts.push_back(cl[k]);
        }
    }
    std::sort(verts.begin(), verts.end());
    auto last = std::unique(verts.begin(), verts.end());
    verts.resize(last - verts.begin());
    return verts;
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
