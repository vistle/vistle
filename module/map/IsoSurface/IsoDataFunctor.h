#ifndef ISODATAFUNCTOR_H
#define ISODATAFUNCTOR_H

#include <algorithm>
#include <vistle/util/enum.h>
#include <vistle/util/math.h>
#include <vistle/core/scalar.h>
#include <vistle/core/index.h>
#include <vistle/core/vector.h>
#include <vistle/core/parameter.h>
#include <vistle/core/structuredgridbase.h>

namespace vistle {
class Module;
}

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(SurfaceOption, (Plane)(Sphere)(CylinderX)(CylinderY)(CylinderZ))

template<vistle::Index>
inline vistle::Index lerp(vistle::Index a, vistle::Index b, vistle::Scalar t)
{
    return t > 0.5 ? b : a;
}


#ifdef CUTTINGSURFACE
#define IsoDataFunctor CutDataFunctor

//! generate data on the fly for creating cutting surfaces as isosurfaces
struct IsoDataFunctor {
    IsoDataFunctor(const vistle::Vector3 &vertex, const vistle::Vector3 &point, const vistle::Vector3 &direction,
                   const vistle::Scalar *x, const vistle::Scalar *y, const vistle::Scalar *z, int option,
                   bool flip = false)
    : m_dims{0, 0, 0}
    , m_x(x)
    , m_y(y)
    , m_z(z)
    , m_vertex(vertex)
    , m_point(point)
    , m_direction(direction)
    , m_distance(vertex.dot(point))
    , m_option(option)
    , m_radius2((m_option == Sphere ? m_point - m_vertex : m_direction.cross(m_point - m_vertex)).squaredNorm())
    , m_sign(flip ? -1. : 1.)
    , m_coords(true)
    {}

    IsoDataFunctor(const vistle::Vector3 &vertex, const vistle::Vector3 &point, const vistle::Vector3 &direction,
                   const vistle::Index dims[3], const vistle::Scalar *x, const vistle::Scalar *y,
                   const vistle::Scalar *z, int option, bool flip = false)
    : m_dims{dims[0], dims[1], dims[2]}
    , m_x(x)
    , m_y(y)
    , m_z(z)
    , m_vertex(vertex)
    , m_point(point)
    , m_direction(direction)
    , m_distance(vertex.dot(point))
    , m_option(option)
    , m_radius2((m_option == Sphere ? m_point - m_vertex : m_direction.cross(m_point - m_vertex)).squaredNorm())
    , m_sign(flip ? -1. : 1.)
    , m_coords(false)
    {}

    vistle::Scalar operator()(vistle::Index i)
    {
        if (m_coords) {
            vistle::Vector3 coordinates(m_x[i], m_y[i], m_z[i]);
            switch (m_option) {
            case Plane: {
                return m_sign * (m_distance - m_vertex.dot(coordinates));
            }
            case Sphere: {
                return m_sign * ((coordinates - m_vertex).squaredNorm() - m_radius2);
            }
            default: {
                // all cylinders
                return m_sign * ((m_direction.cross(coordinates - m_vertex)).squaredNorm() - m_radius2);
            }
            }
        } else {
            auto idx = vistle::StructuredGridBase::vertexCoordinates(i, m_dims);
            vistle::Vector3 coordinates(m_x[idx[0]], m_y[idx[1]], m_z[idx[2]]);
            switch (m_option) {
            case Plane: {
                return m_sign * (m_distance - m_vertex.dot(coordinates));
            }
            case Sphere: {
                return m_sign * ((coordinates - m_vertex).squaredNorm() - m_radius2);
            }
            default: {
                // all cylinders
                return m_sign * ((m_direction.cross(coordinates - m_vertex)).squaredNorm() - m_radius2);
            }
            }
        }
    }
    const vistle::Index m_dims[3];
    const vistle::Scalar *m_x;
    const vistle::Scalar *m_y;
    const vistle::Scalar *m_z;
    const vistle::Vector3 m_vertex;
    const vistle::Vector3 m_point;
    const vistle::Vector3 m_direction;
    const vistle::Scalar m_distance;
    const int m_option;
    const vistle::Scalar m_radius2;
    const vistle::Scalar m_sign;
    const bool m_coords;
};

#else
//! fetch data from scalar field for generating isosurface
struct IsoDataFunctor {
    IsoDataFunctor(const vistle::Scalar *data): m_volumedata(data) {}
    vistle::Scalar operator()(vistle::Index i) { return m_volumedata[i]; }

    const vistle::Scalar *m_volumedata;
};
#endif

#if defined(CUTGEOMETRYOLD)
#define IsoController CutGeoOldController
#elif defined(CUTGEOMETRY)
#define IsoController GeoController
#elif defined(CUTTINGSURFACE)
#define IsoController CutController
#elif defined(ISOHEIGHTSURFACE)
#define IsoController HeightController
#endif
class IsoController {
public:
    IsoController(vistle::Module *module);
    void init();
    bool changeParameter(const vistle::Parameter *param);
#ifdef CUTTINGSURFACE
    IsoDataFunctor newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *x, const vistle::Scalar *y,
                           const vistle::Scalar *z) const;
    IsoDataFunctor newFunc(const vistle::Matrix4 &objTransform, const vistle::Index dims[3], const vistle::Scalar *x,
                           const vistle::Scalar *y, const vistle::Scalar *z) const;
#else
    IsoDataFunctor newFunc(const vistle::Matrix4 &objTransform, const vistle::Scalar *data) const;
#endif

private:
#ifdef CUTTINGSURFACE
    vistle::Module *m_module = nullptr;
    vistle::IntParameter *m_option = nullptr;
#ifdef TOGGLESIGN
    vistle::IntParameter *m_flip = nullptr;
#endif
#endif
};

#endif
