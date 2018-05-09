#include "cellalgorithm.h"
#include "unstr.h"
#include <util/math.h>

namespace vistle {

// cf. http://stackoverflow.com/questions/808441/inverse-bilinear-interpolation
Vector trilinearInverse(const Vector &pp0, const Vector pp[8]) {
// Computes the inverse of the trilinear map from [0,1]^3 to the box defined
// by points p[0],...,p[7], where the points are ordered consistent with our hexahedron order:
// p[0]~(0,0,0), p[1]~(0,0,1), p[2]~(0,1,1), p[3]~(0,1,0),
// p[4]~(1,0,0), p[5]~(1,0,1), p[6]~(1,1,1), p[7]~(1,1,0)
// Uses Gauss-Newton method. Inputs must be column vectors.

    const int iter = 5;
    const double tol = 1e-10;
    DoubleVector3 ss(0.5, 0.5, 0.5); // initial guess

    DoubleVector3 p0(pp0.cast<double>());
    DoubleVector3 p[8];
    for (int i=0; i<8; ++i)
        p[i] = pp[i].cast<double>();

    const double tol2 = tol*tol;
    for (int k=0; k<iter; ++k) {
       const double s(ss[0]), t(ss[1]), w(ss[2]);
       const DoubleVector3 res
             = p[0]*(1-s)*(1-t)*(1-w) + p[1]*s*(1-t)*(1-w)
             + p[2]*s*t*(1-w)         + p[3]*(1-s)*t*(1-w)
             + p[4]*(1-s)*(1-t)*w     + p[5]*s*(1-t)*w
             + p[6]*s*t*w             + p[7]*(1-s)*t*w
             - p0;

       //std::cerr << "iter: " << k << ", res: " << res.transpose() << ", weights: " << ss.transpose() << std::endl;

       if (res.squaredNorm() < tol2)
          break;

       const auto Js
             = -p[0]*(1-t)*(1-w) + p[1]*(1-t)*(1-w)
             +  p[2]*t*(1-w)     - p[3]*t*(1-w)
             + -p[4]*(1-t)*w     + p[5]*(1-t)*w
             +  p[6]*t*w         - p[7]*t*w;
       const auto Jt
             = -p[0]*(1-s)*(1-w) - p[1]*s*(1-w)
             +  p[2]*s*(1-w)     + p[3]*(1-s)*(1-w)
             + -p[4]*(1-s)*w     - p[5]*s*w
             +  p[6]*s*w         + p[7]*(1-s)*w;
       const auto Jw
             = -p[0]*(1-s)*(1-t) - p[1]*s*(1-t)
             + -p[2]*s*t         - p[3]*(1-s)*t
             +  p[4]*(1-s)*(1-t) + p[5]*s*(1-t)
             +  p[6]*s*t         + p[7]*(1-s)*t;
       DoubleMatrix3 J;
       J << Js, Jt, Jw;
       //ss = ss - (J'*J)\(J'*r);
       ss -= (J.transpose()*J).llt().solve(J.transpose()*res);
       for (int c=0; c<3; ++c)
           ss[c] = clamp(ss[c], double(0), double(1));
    }

    return ss.cast<Scalar>();
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
      if (sign==0) {
          sign = n.dot(corners2[i]);
          if (std::abs(sign) < 1e-5)
              sign = 0;
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

bool insidePolygon(const Vector &point, const Vector *corners, Index nCorners, const Vector &normal) {

   // project into 2D and transform point into origin
   Index max = 2;
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

   // count intersections of edges with positive x-axis
   int nisect = 0;
   for (Index i=0; i<nCorners; ++i) {
      Vector2 c0 = corners2[i];
      Vector2 c1 = corners2[(i+1)%nCorners];
      if (c0[1]<0 && c1[1]<0)
          continue;
      if (c0[1]>0 && c1[1]>0)
          continue;
      if (c0[0] < 0 && c1[0] < 0)
          continue;
      if (c0[0]>=0 && c1[0]>=0) {
          ++nisect;
          continue;
      }
      Scalar mInv = (c1[0]-c0[0])/(c1[1]-c0[1]);
      Scalar x=c0[1]*mInv+c0[0];
      if (x >= 0)
          ++nisect;
   }

   return (nisect%2) == 1;
}

std::pair<Vector,Vector> faceNormalAndCenter(unsigned char type, Index f, const Index *cl, const Scalar *x, const Scalar *y, const Scalar *z) {
    const auto &faces = UnstructuredGrid::FaceVertices[type];
    const auto &sizes = UnstructuredGrid::FaceSizes[type];
    const auto &face = faces[f];
    Vector normal(0,0,0);
    Vector center(0,0,0);
    int N = sizes[f];
    assert(N==3 || N==4);
    Index v = cl[face[N-1]];
    Vector c0(x[v], y[v], z[v]);
    for (int i=0; i<N; ++i) {
        Index v = cl[face[i]];
        Vector c1(x[v], y[v], z[v]);
        center += c1;
        normal += c0.cross(c1);
        c0 = c1;
    }
    return std::make_pair(normal.normalized(), center/N);
}

std::pair<Vector,Vector> faceNormalAndCenter(Index nVert, const Index *verts, const Scalar *x, const Scalar *y, const Scalar *z) {
    Vector normal(0,0,0);
    Vector center(0,0,0);
    assert(nVert >= 3);
    if (nVert < 3)
        return std::make_pair(normal, center);

    Index v = verts[nVert-1];
    Vector c0(x[v], y[v], z[v]);
    for (Index i=0; i<nVert; ++i) {
        Index v = verts[i];
        Vector c1(x[v], y[v], z[v]);
        center += c1;
        normal += c0.cross(c1);
        c0 = c1;
    }

    return std::make_pair(normal.normalized(), center/nVert);
}

}
