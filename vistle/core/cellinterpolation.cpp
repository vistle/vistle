#include "cellinterpolation.h"

namespace vistle {

// cf. http://stackoverflow.com/questions/808441/inverse-bilinear-interpolation
Vector trilinearInverse(const Vector &p0, const Vector p[8]) {
// Computes the inverse of the trilinear map from [0,1]^3 to the box defined
// by points p[0],...,p[7], where the points are ordered consistent with our hexahedron order:
// p[0]~(0,0,0), p[1]~(0,0,1), p[2]~(0,1,1), p[3]~(0,1,0),
// p[4]~(1,0,0), p[5]~(1,0,1), p[6]~(1,1,1), p[7]~(1,1,0)
// Uses Gauss-Newton method. Inputs must be column vectors.

    const int iter = 5;
    const Scalar tol = 1e-6;
    Vector ss(0.5, 0.5, 0.5); // initial guess

    const Scalar tol2 = tol*tol;
    for (int k=0; k<iter; ++k) {
       const Scalar s(ss[0]), t(ss[1]), w(ss[2]);
       const Vector res
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
       Matrix3 J;
       J << Js, Jt, Jw;
       //ss = ss - (J'*J)\(J'*r);
       ss -= (J.transpose()*J).llt().solve(J.transpose()*res);
    }

    return ss;
}

}
