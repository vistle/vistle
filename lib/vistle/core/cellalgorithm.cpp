#include "cellalgorithm.h"
#include "unstr.h"
#include "celltypes.h"
#include <vistle/util/math.h>

namespace vistle {

static const Scalar Epsilon = 1e-7;

// cf. http://stackoverflow.com/questions/808441/inverse-bilinear-interpolation
Vector3 trilinearInverse(const Vector3 &pp0, const Vector3 pp[8])
{
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
    for (int i = 0; i < 8; ++i)
        p[i] = pp[i].cast<double>();

    const double tol2 = tol * tol;
    for (int k = 0; k < iter; ++k) {
        const double s(ss[0]), t(ss[1]), w(ss[2]);
        const DoubleVector3 res = p[0] * (1 - s) * (1 - t) * (1 - w) + p[1] * s * (1 - t) * (1 - w) +
                                  p[2] * s * t * (1 - w) + p[3] * (1 - s) * t * (1 - w) + p[4] * (1 - s) * (1 - t) * w +
                                  p[5] * s * (1 - t) * w + p[6] * s * t * w + p[7] * (1 - s) * t * w - p0;

        //std::cerr << "iter: " << k << ", res: " << res.transpose() << ", weights: " << ss.transpose() << std::endl;

        if (res.squaredNorm() < tol2)
            break;

        const auto Js = -p[0] * (1 - t) * (1 - w) + p[1] * (1 - t) * (1 - w) + p[2] * t * (1 - w) - p[3] * t * (1 - w) +
                        -p[4] * (1 - t) * w + p[5] * (1 - t) * w + p[6] * t * w - p[7] * t * w;
        const auto Jt = -p[0] * (1 - s) * (1 - w) - p[1] * s * (1 - w) + p[2] * s * (1 - w) + p[3] * (1 - s) * (1 - w) +
                        -p[4] * (1 - s) * w - p[5] * s * w + p[6] * s * w + p[7] * (1 - s) * w;
        const auto Jw = -p[0] * (1 - s) * (1 - t) - p[1] * s * (1 - t) + -p[2] * s * t - p[3] * (1 - s) * t +
                        p[4] * (1 - s) * (1 - t) + p[5] * s * (1 - t) + p[6] * s * t + p[7] * (1 - s) * t;
        DoubleMatrix3 J;
        J << Js, Jt, Jw;
        //ss = ss - (J'*J)\(J'*r);
        ss -= (J.transpose() * J).llt().solve(J.transpose() * res);
        for (int c = 0; c < 3; ++c)
            ss[c] = clamp(ss[c], double(0), double(1));
    }

    return ss.cast<Scalar>();
}

bool insideConvexPolygon(const Vector3 &point, const Vector3 *corners, Index nCorners, const Vector3 &normal)
{
    // project into 2D and transform point into origin
    Index max = 0;
    normal.cwiseAbs().maxCoeff(&max);
    int c1 = 0, c2 = 1;
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
    for (Index i = 0; i < nCorners; ++i) {
        corners2[i] << corners[i][c1], corners[i][c2];
        corners2[i] -= point2;
    }

    // check whether edges pass the origin at the same side
    Scalar sign(0);
    for (Index i = 0; i < nCorners; ++i) {
        Vector2 n = corners2[(i + 1) % nCorners] - corners2[i];
        std::swap(n[0], n[1]);
        n[0] *= -1;
        if (sign == 0) {
            sign = n.dot(corners2[i]);
            if (std::abs(sign) < 1e-5)
                sign = 0;
        } else if (n.dot(corners2[i]) * sign < 0) {
#ifdef INTERPOL_DEBUG
            std::cerr << "outside: normal: " << normal.transpose() << ", point: " << point.transpose()
                      << ", p2: " << point2.transpose() << std::endl;
#endif
            return false;
        }
    }

#ifdef INTERPOL_DEBUG
    std::cerr << "inside: normal: " << normal.transpose() << ", point: " << point.transpose()
              << ", p2: " << point2.transpose() << std::endl;
#endif

    return true;
}

/* return whether origin is left (<0), on (=0), or right (>0) of infinite line through p0 and p1 */
static Scalar originSideOfLineZ2D(const Vector3 &p0, const Vector3 &p1)
{
    //return p0[0] * (p1[1] - p0[1]) - p0[1] * (p1[0] - p0[0]);
    return difference_of_products(p0[0], p1[1] - p0[1], p0[1], p1[0] - p0[0]);
}

bool originInsidePolygonZ2D(const Vector3 *corners, Index nCorners)
{
    // based on http://geomalgorithms.com/a03-_inclusion.html

    int wn = 0; // the  winding number counter

    // loop through all edges of the polygon
    for (unsigned i = 0; i < nCorners; i++) { // edge from V[i] to  V[i+1]
        unsigned next = i + 1 < nCorners ? i + 1 : 0;
        if (corners[i][1] <= 0) { // start y <= P.y
            if (corners[next][1] > 0) // an upward crossing
                if (originSideOfLineZ2D(corners[i], corners[next]) > 0) // P left of  edge
                    ++wn; // have  a valid up intersect
        } else { // start y > P.y (no test needed)
            if (corners[next][1] <= 0) // a downward crossing
                if (originSideOfLineZ2D(corners[i], corners[next]) < 0) // P right of  edge
                    --wn; // have  a valid down intersect
        }
    }
    return wn != 0;
}

bool insidePolygon(const Vector3 &point, const Vector3 *corners, Index nCorners, const Vector3 &normal)
{
    // project into 2D and transform point into origin
    Index max = 2;
    normal.cwiseAbs().maxCoeff(&max);
    int c1 = 0, c2 = 1;
    if (max == 0) {
        c1 = 1;
        c2 = 2;
    } else if (max == 1) {
        c1 = 0;
        c2 = 2;
    }
    Vector2 point2;
    point2 << point[c1], point[c2];
    int nisect = 0;
    if (nCorners <= 4) {
        Vector2 corners2[4];
        for (Index i = 0; i < nCorners; ++i) {
            corners2[i] = Vector2(corners[i][c1], corners[i][c2]) - point2;
        }

        // count intersections of edges with positive x-axis
        for (Index i = 0; i < nCorners; ++i) {
            const auto &c0 = corners2[i];
            const auto &c1 = corners2[(i + 1) % nCorners];
            if (c0[1] < 0 && c1[1] < 0)
                continue;
            if (c0[1] > 0 && c1[1] > 0)
                continue;
            if (c0[0] < 0 && c1[0] < 0)
                continue;
            if (c0[0] >= 0 && c1[0] >= 0) {
                ++nisect;
                continue;
            }
            if (c1[1] > c0[1]) {
                if (c0[0] * (c1[0] - c0[0]) > -c0[0] * (c1[1] - c0[1]))
                    ++nisect;
            } else {
                if (c0[0] * (c1[0] - c0[0]) < -c0[0] * (c1[1] - c0[1]))
                    ++nisect;
            }
        }
    } else {
        std::vector<Vector2> corners2(nCorners);
        for (Index i = 0; i < nCorners; ++i) {
            corners2[i] = Vector2(corners[i][c1], corners[i][c2]) - point2;
        }

        // count intersections of edges with positive x-axis
        for (Index i = 0; i < nCorners; ++i) {
            Vector2 c0 = corners2[i];
            Vector2 c1 = corners2[(i + 1) % nCorners];
            if (c0[1] < 0 && c1[1] < 0)
                continue;
            if (c0[1] > 0 && c1[1] > 0)
                continue;
            if (c0[0] < 0 && c1[0] < 0)
                continue;
            if (c0[0] >= 0 && c1[0] >= 0) {
                ++nisect;
                continue;
            }
            if (c1[1] > c0[1]) {
                if (c0[0] * (c1[0] - c0[0]) > -c0[0] * (c1[1] - c0[1]))
                    ++nisect;
            } else {
                if (c0[0] * (c1[0] - c0[0]) < -c0[0] * (c1[1] - c0[1]))
                    ++nisect;
            }
        }
    }

    return (nisect % 2) == 1;
}

std::pair<Vector3, Vector3> faceNormalAndCenter(Byte type, Index f, const Vector3 *corners)
{
    const auto &faces = UnstructuredGrid::FaceVertices[type];
    const auto &sizes = UnstructuredGrid::FaceSizes[type];
    const auto &face = faces[f];
    Vector3 normal(0, 0, 0);
    Vector3 center(0, 0, 0);
    int N = sizes[f];
    assert(N == 3 || N == 4);
    Index v = face[N - 1];
    const Vector3 *c0 = &corners[v];
    for (int i = 0; i < N; ++i) {
        Index v = face[i];
        const Vector3 *c1 = &corners[v];
        center += *c1;
        normal += cross(*c0, *c1);
        c0 = c1;
    }
    return std::make_pair(normal.normalized(), center / N);
}

std::pair<Vector3, Vector3> faceNormalAndCenter(Byte type, Index f, const Index *cl, const Scalar *x, const Scalar *y,
                                                const Scalar *z)
{
    const auto &faces = UnstructuredGrid::FaceVertices[type];
    const auto &sizes = UnstructuredGrid::FaceSizes[type];
    const auto &face = faces[f];
    Vector3 normal(0, 0, 0);
    Vector3 center(0, 0, 0);
    int N = sizes[f];
    assert(N == 3 || N == 4);
    Index v = cl[face[N - 1]];
    Vector3 c0(x[v], y[v], z[v]);
    for (int i = 0; i < N; ++i) {
        Index v = cl[face[i]];
        Vector3 c1(x[v], y[v], z[v]);
        center += c1;
        normal += cross(c0, c1);
        c0 = c1;
    }
    return std::make_pair(normal.normalized(), center / N);
}

std::pair<Vector3, Vector3> faceNormalAndCenter(Index nVert, const Index *verts, const Scalar *x, const Scalar *y,
                                                const Scalar *z)
{
    Vector3 normal(0, 0, 0);
    Vector3 center(0, 0, 0);
    assert(nVert >= 3);
    if (nVert < 3)
        return std::make_pair(normal, center);

    Index v = verts[nVert - 1];
    Vector3 c0(x[v], y[v], z[v]);
    for (Index i = 0; i < nVert; ++i) {
        Index v = verts[i];
        Vector3 c1(x[v], y[v], z[v]);
        center += c1;
        normal += cross(c0, c1);
        c0 = c1;
    }

    return std::make_pair(normal.normalized(), center / nVert);
}

std::pair<Vector3, Vector3> faceNormalAndCenter(Index nVert, const Vector3 *corners)
{
    Vector3 normal(0, 0, 0);
    Vector3 center(0, 0, 0);
    assert(nVert >= 3);
    if (nVert < 3)
        return std::make_pair(normal, center);

    Vector3 c0(corners[nVert - 1]);
    for (Index i = 0; i < nVert; ++i) {
        Vector3 c1(corners[i]);
        center += c1;
        normal += cross(c0, c1);
        c0 = c1;
    }

    return std::make_pair(normal.normalized(), center / nVert);
}

bool insideCell(const Vector3 &point, Byte type, Index nverts, const Index *cl, const Scalar *x, const Scalar *y,
                const Scalar *z)
{
    const Vector3 zaxis(0, 0, 1);

    type &= cell::TYPE_MASK;
    switch (type) {
    case cell::TETRAHEDRON:
    case cell::PYRAMID:
    case cell::PRISM:
    case cell::HEXAHEDRON: {
        unsigned insideCount = 0;
        const auto numFaces = UnstructuredGrid::NumFaces[type];

        // count intersections of ray from origin along positive z-axis with polygon translated by -point

#ifdef INSIDE_DEBUG
        std::cerr << "POINT: " << point.transpose() << ", #faces=" << numFaces << ", type=" << type << std::endl;
#endif
        Vector3 corners[4];
        for (int f = 0; f < numFaces; ++f) {
            const unsigned nCorners = UnstructuredGrid::FaceSizes[type][f];
            if (nCorners == 0)
                continue;
            Vector3 min, max;
            for (unsigned i = 0; i < nCorners; ++i) {
                const Index v = cl[UnstructuredGrid::FaceVertices[type][f][i]];
                corners[i] = Vector3(x[v], y[v], z[v]) - point;
#ifdef INSIDE_DEBUG
                std::cerr << "   " << corners[i].transpose() << std::endl;
#endif
                if (i == 0) {
                    min = max = corners[0];
                } else {
                    for (int c = 0; c < 3; ++c) {
                        min[c] = std::min(min[c], corners[i][c]);
                        max[c] = std::max(max[c], corners[i][c]);
                    }
                }
            }

#ifdef INSIDE_DEBUG
            std::cerr << "bbox: " << min.transpose() << " -> " << max.transpose() << std::endl;
            std::cerr << "thick: " << (max - min).transpose() << std::endl;
#endif

            if (max[2] < 0) {
                // face is in negative z-axis direction
                continue;
            }
            // z-axis-ray does not intersect bounding rectangle of face
            if (max[0] < 0)
                continue;
            if (min[0] > 0)
                continue;
            if (max[1] < 0)
                continue;
            if (min[1] > 0)
                continue;

            if (originInsidePolygonZ2D(corners, nCorners)) {
                if (min[2] > 0) {
                    ++insideCount;
                    continue;
                } else {
                    const auto nc = faceNormalAndCenter(nCorners, corners);
                    auto &normal = nc.first;
                    auto &center = nc.second;

                    auto ndz = normal.dot(zaxis);
                    if (std::abs(ndz) < Epsilon) {
#ifdef INSIDE_DEBUG
                        std::cerr << "  SKIP" << f << ": parallel" << std::endl;
#endif
                        continue;
                    }

                    auto d = normal.dot(center) / ndz;
                    if (d > 0)
                        ++insideCount;

#ifdef INSIDE_DEBUG
                    std::cerr << "normal: " << normal.transpose() << ", d: " << d << ", insideCount: " << insideCount
                              << std::endl;
#endif
                }
#ifdef INSIDE_DEBUG
            } else {
                std::cerr << "  SKIP" << f << ": not inside" << std::endl;
#endif
            }
        }
#ifdef INSIDE_DEBUG
        std::cerr << "INSIDECOUNT: " << insideCount << std::endl;
#endif

        return insideCount % 2;
        break;
    }
    case UnstructuredGrid::POLYHEDRON: {
        std::vector<Vector3> corners;

        int insideCount = 0;
        Index facestart = InvalidIndex;
        Index term = 0;
        for (Index i = 0; i < nverts; ++i) {
            if (facestart == InvalidIndex) {
                facestart = i;
                term = cl[i];
            } else if (cl[i] == term) {
                const Index nCorners = i - facestart;
                if (nCorners == 0)
                    continue;

                corners.resize(nCorners);
                Vector3 min, max;
                for (Index k = 0; k < nCorners; ++k) {
                    const Index v = cl[facestart + k];
                    corners[k] = Vector3(x[v], y[v], z[v]) - point;
                    if (k == 0) {
                        min = max = corners[0];
                    } else {
                        for (int c = 0; c < 3; ++c) {
                            min[c] = std::min(min[c], corners[k][c]);
                            max[c] = std::max(max[c], corners[k][c]);
                        }
                    }
                }
                facestart = InvalidIndex;

                if (max[2] < 0)
                    continue;
                if (max[0] < 0)
                    continue;
                if (min[0] > 0)
                    continue;
                if (max[1] < 0)
                    continue;
                if (min[1] > 0)
                    continue;

                if (originInsidePolygonZ2D(corners.data(), nCorners)) {
                    if (min[2] > 0) {
                        ++insideCount;
                        continue;
                    } else {
                        const auto nc = faceNormalAndCenter(nCorners, corners.data());
                        auto &normal = nc.first;
                        auto &center = nc.second;

                        auto ndz = normal.dot(zaxis);
                        if (std::abs(ndz) < Epsilon) {
                            continue;
                        }

                        auto d = normal.dot(center) / ndz;
                        if (d > 0)
                            ++insideCount;
                    }
                }
            }
        }

        return insideCount % 2;
        break;
    }
    default:
        std::cerr << "insideCell: unhandled cell type " << type << std::endl;
        return false;
    }

    return false;
}

} // namespace vistle
