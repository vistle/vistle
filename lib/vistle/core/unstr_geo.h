#ifndef VISTLE_CORE_UNSTR_GEO_H
#define VISTLE_CORE_UNSTR_GEO_H

#include "unstr.h"

namespace vistle {

template<std::size_t N, typename F>
void for_(F func)
{
    for_(func, std::make_index_sequence<N>());
}

typedef std::array<const Scalar *, 3> XYZArray;

Vector3 getVecFromXYZArray(const XYZArray &XYZArray, Index i)
{
    return Vector3(XYZArray[0][i], XYZArray[1][i], XYZArray[2][i]);
}

template<UnstructuredGrid::Type T>
Scalar edgeLength(Index numVerts, const Index *cl, const XYZArray &XYZArray);

template<>
Scalar edgeLength<UnstructuredGrid::NONE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar edgeLength<UnstructuredGrid::POINT>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar edgeLength<UnstructuredGrid::BAR>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    assert(numVerts == UnstructuredGrid::NumVertices[UnstructuredGrid::BAR]);
    return (getVecFromXYZArray(XYZArray, cl[1]) - getVecFromXYZArray(XYZArray, cl[0])).norm();
}

template<>
Scalar edgeLength<UnstructuredGrid::POLYLINE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    if (numVerts < 1)
        return 0;
    Scalar len = 0;
    for (Index i = 0; i < numVerts - 1; ++i) {
        len += edgeLength<UnstructuredGrid::BAR>(2, cl + i, XYZArray);
    }
    return len;
}

template<>
Scalar edgeLength<UnstructuredGrid::POLYGON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    if (numVerts < 0)
        return 0;
    Scalar len = edgeLength<UnstructuredGrid::POLYLINE>(numVerts, cl, XYZArray);
    len += (getVecFromXYZArray(XYZArray, cl[0]) - getVecFromXYZArray(XYZArray, cl[numVerts - 1])).norm();
    return len;
}

template<>
Scalar edgeLength<UnstructuredGrid::TRIANGLE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return edgeLength<UnstructuredGrid::POLYGON>(numVerts, cl, XYZArray);
}

template<>
Scalar edgeLength<UnstructuredGrid::QUAD>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return edgeLength<UnstructuredGrid::POLYGON>(numVerts, cl, XYZArray);
}

template<>
Scalar edgeLength<UnstructuredGrid::POLYHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    Scalar len = 0;
    Index start = InvalidIndex;
    for (Index v = 0; v < numVerts; ++v) {
        if (start == InvalidIndex) {
            start = cl[v];
        } else if (cl[v] == start) {
            start = InvalidIndex;
            continue;
        }
        len += edgeLength<UnstructuredGrid::BAR>(2, cl + v, XYZArray);
    }
    return len;
}

template<UnstructuredGrid::Type T>
std::array<Vector3, UnstructuredGrid::NumVertices[T]> getVertices(const Index *cl, const XYZArray &XYZArray)
{
    std::array<Vector3, UnstructuredGrid::NumVertices[T]> a;
    for (int i = 0; i < UnstructuredGrid::NumVertices[T]; ++i) {
        a[i] = getVecFromXYZArray(XYZArray, cl[i]);
    }
    return a;
}

template<>
Scalar edgeLength<UnstructuredGrid::TETRAHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    auto p = getVertices<UnstructuredGrid::TETRAHEDRON>(cl, XYZArray);
    return (p[0] - p[1]).norm() + (p[0] - p[2]).norm() + (p[0] - p[3]).norm() + (p[1] - p[2]).norm() +
           (p[1] - p[3]).norm() + (p[2] - p[3]).norm();
}

template<>
Scalar edgeLength<UnstructuredGrid::HEXAHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    auto p = getVertices<UnstructuredGrid::HEXAHEDRON>(cl, XYZArray);
    return (p[0] - p[1]).norm() + (p[1] - p[2]).norm() + (p[2] - p[3]).norm() + (p[3] - p[0]).norm() + //bottom quad
           (p[4] - p[5]).norm() + (p[5] - p[6]).norm() + (p[6] - p[7]).norm() + (p[7] - p[4]).norm() + //top quad
           (p[0] - p[4]).norm() + (p[1] - p[5]).norm() + (p[2] - p[6]).norm() + (p[3] - p[7]).norm(); //connections
}

template<>
Scalar edgeLength<UnstructuredGrid::PRISM>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    auto p = getVertices<UnstructuredGrid::PRISM>(cl, XYZArray);
    return (p[0] - p[1]).norm() + (p[1] - p[2]).norm() + (p[2] - p[0]).norm() + //bottom triangle
           (p[3] - p[4]).norm() + (p[4] - p[5]).norm() + (p[5] - p[3]).norm() + //top triangle
           (p[0] - p[3]).norm() + (p[1] - p[4]).norm() + (p[2] - p[5]).norm(); //connections
}

template<>
Scalar edgeLength<UnstructuredGrid::PYRAMID>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    auto p = getVertices<UnstructuredGrid::PYRAMID>(cl, XYZArray);
    return (p[0] - p[1]).norm() + (p[1] - p[2]).norm() + (p[2] - p[3]).norm() + (p[3] - p[0]).norm() + //bottom quad
           (p[0] - p[4]).norm() + (p[1] - p[4]).norm() + (p[2] - p[4]).norm() + (p[3] - p[4]).norm(); //connections
}


template<UnstructuredGrid::Type T>
Scalar surface(Index numVerts, const Index *cl, const XYZArray &XYZArray);

template<>
Scalar surface<UnstructuredGrid::NONE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar surface<UnstructuredGrid::POINT>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar surface<UnstructuredGrid::BAR>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar surface<UnstructuredGrid::POLYLINE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar surface<UnstructuredGrid::TRIANGLE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    auto p = getVertices<UnstructuredGrid::TRIANGLE>(cl, XYZArray);
    return Scalar(0.5) * (p[1] - p[0]).cross(p[2] - p[0]).norm();
}

template<>
Scalar surface<UnstructuredGrid::POLYGON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    Scalar retval = 0;
    for (Index i = 0; i < numVerts; ++i) {
        auto p0 = getVecFromXYZArray(XYZArray, cl[i]);
        auto p1 = getVecFromXYZArray(XYZArray, cl[(i + 1) % numVerts]);
        auto p2 = getVecFromXYZArray(XYZArray, cl[(i + 2) % numVerts]);
        retval += Scalar(0.5) * (p1 - p0).cross(p2 - p0).norm();
    }
    return retval;
}

template<>
Scalar surface<UnstructuredGrid::QUAD>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::QUAD>(cl, XYZArray);
    return Scalar(0.5) * (p[1] - p[0]).cross(p[2] - p[0]).norm() +
           Scalar(0.5) * (p[2] - p[0]).cross(p[3] - p[0]).norm();
}

template<>
Scalar surface<UnstructuredGrid::TETRAHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::TETRAHEDRON>(cl, XYZArray);
    return Scalar(0.5) * ((p[1] - p[0]).cross(p[2] - p[0]).norm() + (p[2] - p[0]).cross(p[3] - p[0]).norm() +
                          (p[3] - p[0]).cross(p[1] - p[0]).norm() + (p[2] - p[1]).cross(p[3] - p[1]).norm());
}

template<>
Scalar surface<UnstructuredGrid::POLYHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    Scalar retval = 0;
    const Index *polygonCl = cl;
    for (Index i = 0; i < numVerts; ++i) {
        Index numVertsOfPolygon = 0;
        Index firstVert = polygonCl[0];
        while (polygonCl[numVertsOfPolygon] != firstVert) {
            ++numVertsOfPolygon;
        }
        retval += surface<UnstructuredGrid::POLYGON>(numVertsOfPolygon, polygonCl, XYZArray);
        polygonCl += numVertsOfPolygon;
        i += numVertsOfPolygon;
    }
    return retval;
}

template<>
Scalar surface<UnstructuredGrid::HEXAHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::HEXAHEDRON>(cl, XYZArray);
    return (p[1] - p[0]).cross(p[2] - p[0]).norm() + (p[2] - p[0]).cross(p[3] - p[0]).norm() +
           (p[3] - p[0]).cross(p[0] - p[4]).norm() + (p[4] - p[0]).cross(p[5] - p[0]).norm() +
           (p[5] - p[0]).cross(p[1] - p[0]).norm() + (p[1] - p[0]).cross(p[0] - p[4]).norm();
}
template<>
Scalar surface<UnstructuredGrid::PRISM>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::PRISM>(cl, XYZArray);
    return Scalar(0.5) * ((p[1] - p[0]).cross(p[2] - p[0]).norm() + (p[2] - p[0]).cross(p[3] - p[0]).norm() +
                          (p[3] - p[0]).cross(p[0] - p[4]).norm() + (p[4] - p[0]).cross(p[5] - p[0]).norm() +
                          (p[5] - p[0]).cross(p[1] - p[0]).norm());
}

template<>
Scalar surface<UnstructuredGrid::PYRAMID>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::PYRAMID>(cl, XYZArray);
    return Scalar(0.5) * ((p[1] - p[0]).cross(p[2] - p[0]).norm() + (p[2] - p[0]).cross(p[3] - p[0]).norm() +
                          (p[1] - p[0]).cross(p[4] - p[0]).norm() + (p[2] - p[1]).cross(p[4] - p[1]).norm() +
                          (p[3] - p[2]).cross(p[4] - p[2]).norm() + (p[3] - p[0]).cross(p[4] - p[0]).norm());
}

template<UnstructuredGrid::Type T>
Scalar volume(Index numVerts, const Index *cl, const XYZArray &XYZArray);

template<>
Scalar volume<UnstructuredGrid::NONE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar volume<UnstructuredGrid::POINT>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar volume<UnstructuredGrid::BAR>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar volume<UnstructuredGrid::POLYLINE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar volume<UnstructuredGrid::TRIANGLE>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar volume<UnstructuredGrid::QUAD>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

template<>
Scalar volume<UnstructuredGrid::POLYGON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    return 0;
}

Scalar tripleProduct(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p3)
{
    return std::abs((p1 - p0).dot((p2 - p0).cross(p3 - p0)));
}

template<>
Scalar volume<UnstructuredGrid::TETRAHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::TETRAHEDRON>(cl, XYZArray);
    return tripleProduct(p[0], p[1], p[2], p[3]) / Scalar(6.0);
}

template<>
Scalar volume<UnstructuredGrid::POLYHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    Scalar volume = 0;
    Index term = InvalidIndex;
    Index faceBegin = 0;
    const Vector3 p0 = getVecFromXYZArray(XYZArray, cl[0]);

    for (Index i = 0; i < numVerts; ++i) {
        if (term == InvalidIndex) {
            term = cl[i];
        } else if (cl[i] == term) {
            for (size_t j = faceBegin + 1; j < i - 1; j++) {
                volume +=
                    tripleProduct(p0, getVecFromXYZArray(XYZArray, cl[j]), getVecFromXYZArray(XYZArray, cl[j + 1]),
                                  getVecFromXYZArray(XYZArray, cl[faceBegin]));
            }

            faceBegin = i;
            term = InvalidIndex;
        }
    }
    return volume / Scalar(6.0);
}

template<>
Scalar volume<UnstructuredGrid::HEXAHEDRON>(Index numVerts, const Index *cl, const XYZArray &XYZArray)

{
    const auto p = getVertices<UnstructuredGrid::HEXAHEDRON>(cl, XYZArray);
    return (tripleProduct(p[0], p[1], p[2], p[5]) + tripleProduct(p[0], p[2], p[6], p[5]) +
            tripleProduct(p[0], p[5], p[6], p[4]) + tripleProduct(p[0], p[3], p[2], p[7]) +
            tripleProduct(p[0], p[2], p[6], p[7]) + tripleProduct(p[0], p[7], p[6], p[4])) /
           Scalar(6);
}

template<>
Scalar volume<UnstructuredGrid::PRISM>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::PRISM>(cl, XYZArray);
    return Scalar(0.5) * (p[1] - p[0]).cross(p[2] - p[0]).norm() * (p[3] - p[0]).norm();
}

template<>
Scalar volume<UnstructuredGrid::PYRAMID>(Index numVerts, const Index *cl, const XYZArray &XYZArray)
{
    const auto p = getVertices<UnstructuredGrid::PYRAMID>(cl, XYZArray);
    return surface<UnstructuredGrid::QUAD>(4, cl, XYZArray) * (p[0] - p[1]).normalized().cross(p[0] - p[4]).norm() /
           Scalar(3.0);
}

} // namespace vistle
#endif
