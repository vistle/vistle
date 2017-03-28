#ifndef ISODATAFUNCTOR_H
#define ISODATAFUNCTOR_H

#include <algorithm>
#include <thrust/execution_policy.h>
#include <util/enum.h>
#include <util/math.h>
#include <core/scalar.h>
#include <core/index.h>
#include <core/vector.h>
#include <core/parameter.h>

namespace vistle {
class Module;
}

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(SurfaceOption,
   (Plane)
   (Sphere)
   (CylinderX)
   (CylinderY)
   (CylinderZ)
)

template<vistle::Index>
inline vistle::Index lerp(vistle::Index a, vistle::Index b, vistle::Scalar t) {
    return t > 0.5 ? b : a;
}


const vistle::Scalar EPSILON = 1.0e-10f;

inline vistle::Scalar __host__ __device__ tinterp(vistle::Scalar iso, const vistle::Scalar &f0, const vistle::Scalar &f1) {

   const vistle::Scalar diff = (f1 - f0);
   const vistle::Scalar d0 = iso - f0;
   if (fabs(diff) < EPSILON) {
       const vistle::Scalar d1 = f1 - iso;
      return fabs(d0) < fabs(d1) ? 1 : 0;
   }

   return std::min(vistle::Scalar(1), std::max(vistle::Scalar(0), d0 / diff));
}

#ifdef CUTTINGSURFACE

//! generate data on the fly for creating cutting surfaces as isosurfaces
struct IsoDataFunctor {

   IsoDataFunctor(const vistle::Vector &vertex, const vistle::Vector &point, const vistle::Vector &direction, const vistle::Scalar* x, const vistle::Scalar* y, const vistle::Scalar* z, int option, bool flip=false)
      : m_x(x)
      , m_y(y)
      , m_z(z)
      , m_vertex(vertex)
      , m_point(point)
      , m_direction(direction)
      , m_distance(vertex.dot(point))
      , m_option(option)
      , m_radius2((m_option==Sphere ? m_point-m_vertex : m_direction.cross(m_point-m_vertex)).squaredNorm())
      , m_sign(flip ? -1. : 1.)
   {}

   __host__ __device__ vistle::Scalar operator()(vistle::Index i) {
      vistle::Vector coordinates(m_x[i], m_y[i], m_z[i]);
      switch(m_option) {
         case Plane: {
            return m_sign*(m_distance - m_vertex.dot(coordinates));
         }
         case Sphere: {
            return m_sign*((coordinates-m_vertex).squaredNorm() - m_radius2);
         }
         default: {
            // all cylinders
            return m_sign*((m_direction.cross(coordinates - m_vertex)).squaredNorm() - m_radius2);
         }
      }
   }
   const vistle::Scalar* m_x;
   const vistle::Scalar* m_y;
   const vistle::Scalar* m_z;
   const vistle::Vector m_vertex;
   const vistle::Vector m_point;
   const vistle::Vector m_direction;
   const vistle::Scalar m_distance;
   const int m_option;
   const vistle::Scalar m_radius2;
   const vistle::Scalar m_sign;
};

#else
//! fetch data from scalar field for generating isosurface
struct IsoDataFunctor {

   IsoDataFunctor(const vistle::Scalar *data)
      : m_volumedata(data)
   {}
   __host__ __device__ vistle::Scalar operator()(vistle::Index i) { return m_volumedata[i]; }

   const vistle::Scalar *m_volumedata;

};
#endif

class IsoController {

public:
    IsoController(vistle::Module *module);
    bool changeParameter(const vistle::Parameter* param);
#ifdef CUTTINGSURFACE
    IsoDataFunctor newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *x, const vistle::Scalar *y, const vistle::Scalar *z) const;
#else
    IsoDataFunctor newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *data) const;
#endif

private:
#ifdef CUTTINGSURFACE
    vistle::Module *m_module;
    vistle::IntParameter *m_option;
#ifdef TOGGLESIGN
    vistle::IntParameter *m_flip;
#endif
#endif
};

#endif
