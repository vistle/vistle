#include "unstr.h"
#include "unstr_impl.h"
#include "archives.h"
#include <cassert>
#include <algorithm>
#include "cellalgorithm.h"
#include <set>

//#define INTERPOL_DEBUG
//#define INSIDE_DEBUG

namespace vistle {

static const Scalar Epsilon = 1e-7;

/* cell types:
 NONE        =  0,
 BAR         =  1,
 TRIANGLE    =  2,
 QUAD        =  3,
 TETRAHEDRON =  4,
 PYRAMID     =  5,
 PRISM       =  6,
 HEXAHEDRON  =  7,
 VPOLYHEDRON =  8,
 POLYGON     =  9,
 POINT       = 10,
 CPOLYHEDRON = 11,
 */

const int UnstructuredGrid::Dimensionality[UnstructuredGrid::NUM_TYPES] = {-1, 1, 2, 2, 3, 3, 3, 3, -1, -1, 0, 3};

const int UnstructuredGrid::NumVertices[UnstructuredGrid::NUM_TYPES] = {0, 2, 3, 4, 4, 5, 6, 8, -1, -1, 1, -1};
const int UnstructuredGrid::NumFaces[UnstructuredGrid::NUM_TYPES] = {0, 0, 1, 1, 4, 5, 5, 6, -1, -1, 0, -1};

const unsigned UnstructuredGrid::FaceSizes[UnstructuredGrid::NUM_TYPES][UnstructuredGrid::MaxNumFaces] = {
    // none
    {0, 0, 0, 0, 0, 0},
    // bar
    {0, 0, 0, 0, 0, 0},
    // triangle
    {3, 0, 0, 0, 0, 0},
    // quad
    {4, 0, 0, 0, 0, 0},
    // tetrahedron
    {3, 3, 3, 3, 0, 0},
    // pyramid
    {4, 3, 3, 3, 0, 0},
    // prism
    {3, 4, 4, 4, 3, 0},
    // hexahedron
    {4, 4, 4, 4, 4, 4},
};

// clang-format off
const unsigned UnstructuredGrid::FaceVertices[UnstructuredGrid::NUM_TYPES][UnstructuredGrid::MaxNumFaces][UnstructuredGrid::MaxNumVertices] = {
{ // none
},
{ // bar
},
{ // triangle
  { 2, 1, 0 },
},
{ // quad
   { 3, 2, 1, 0 },
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
// clang-format on

UnstructuredGrid::UnstructuredGrid(const Index numElements, const Index numCorners, const Index numVertices,
                                   const Meta &meta)
: UnstructuredGrid::Base(UnstructuredGrid::Data::create(numElements, numCorners, numVertices, meta))
{
    refreshImpl();
}

void UnstructuredGrid::resetElements()
{
    Base::resetElements();

    d()->tl = ShmVector<Byte>();
    d()->tl.construct(0);
}

bool UnstructuredGrid::isEmpty()
{
    return Base::isEmpty();
}

bool UnstructuredGrid::isEmpty() const
{
    return Base::isEmpty();
}

bool UnstructuredGrid::checkImpl() const
{
    V_CHECK(d()->tl->check());
    V_CHECK(d()->tl->size() == getNumElements());
    return true;
}

bool UnstructuredGrid::isConvex(const Index elem) const
{
    if (elem == InvalidIndex)
        return false;
    return tl()[elem] & CONVEX_BIT || (tl()[elem] & TYPE_MASK) == TETRAHEDRON;
}

bool UnstructuredGrid::isGhostCell(const Index elem) const
{
    if (elem == InvalidIndex)
        return false;
    return tl()[elem] & GHOST_BIT;
}

Index UnstructuredGrid::checkConvexity()
{
    const Scalar Tolerance = 1e-3f;
    const Index nelem = getNumElements();
    auto tl = this->tl().data();
    const auto cl = this->cl().data();
    const auto el = this->el().data();
    const auto x = this->x().data();
    const auto y = this->y().data();
    const auto z = this->z().data();

    Index nonConvexCount = 0;
    for (Index elem = 0; elem < nelem; ++elem) {
        auto type = tl[elem] & TYPE_MASK;
        switch (type) {
        case NONE:
        case BAR:
        case TRIANGLE:
        case POINT:
            tl[elem] |= CONVEX_BIT;
            break;
        case QUAD:
            ++nonConvexCount;
            break;
        case TETRAHEDRON:
            tl[elem] |= CONVEX_BIT;
            break;
        case PRISM:
        case PYRAMID:
        case HEXAHEDRON: {
            bool conv = true;
            const Index begin = el[elem], end = el[elem + 1];
            const Index nvert = end - begin;
            const auto numFaces = NumFaces[type];
            for (int f = 0; f < numFaces; ++f) {
                auto nc = faceNormalAndCenter(type, f, cl + begin, x, y, z);
                auto normal = nc.first;
                auto center = nc.second;
                for (Index idx = 0; idx < nvert; ++idx) {
                    bool check = true;
                    for (Index i = 0; i < FaceSizes[type][f]; ++i) {
                        if (idx == FaceVertices[type][f][i]) {
                            check = false;
                            break;
                        }
                    }

                    if (check) {
                        Index v = cl[begin + idx];
                        const Vector3 p(x[v], y[v], z[v]);
                        if (normal.dot(p - center) > Tolerance) {
                            conv = false;
                            break;
                        }
                    }
                }
                if (!conv)
                    break;
            }
            if (conv) {
                tl[elem] |= CONVEX_BIT;
            } else {
                ++nonConvexCount;
            }
            break;
        }
        case VPOLYHEDRON: {
            bool conv = true;
            std::vector<Index> vert = cellVertices(elem);
            const Index begin = el[elem], end = el[elem + 1];
            const Index nvert = end - begin;
            for (Index i = 0; i < nvert; i += cl[begin + i] + 1) {
                const Index N = cl[begin + i];
                const Index *fl = cl + begin + i + 1;
                auto nc = faceNormalAndCenter(N, fl, x, y, z);
                auto normal = nc.first;
                auto center = nc.second;
                for (auto v: vert) {
                    bool check = true;
                    for (unsigned idx = 0; idx < N; ++idx) {
                        if (v == fl[idx]) {
                            check = false;
                            break;
                        }
                    }

                    if (check) {
                        const Vector3 p(x[v], y[v], z[v]);
                        if (normal.dot(p - center) > Tolerance) {
                            conv = false;
                            break;
                        }
                    }
                }
                if (!conv)
                    break;
            }

            if (conv) {
                tl[elem] |= CONVEX_BIT;
            } else {
                ++nonConvexCount;
            }
            break;
        }
        case CPOLYHEDRON: {
            bool conv = true;
            std::vector<Index> vert = cellVertices(elem);
            const Index begin = el[elem], end = el[elem + 1];
            const Index nvert = end - begin;
            Index facestart = InvalidIndex;
            Index term = 0;
            for (Index i = 0; i < nvert; ++i) {
                if (facestart == InvalidIndex) {
                    facestart = i;
                    term = cl[begin + i];
                } else if (cl[begin + i] == term) {
                    const Index N = i - facestart;
                    const Index *fl = cl + begin + facestart;
                    auto nc = faceNormalAndCenter(N, fl, x, y, z);
                    auto normal = nc.first;
                    auto center = nc.second;
                    for (auto v: vert) {
                        bool check = true;
                        for (unsigned idx = 0; idx < N; ++idx) {
                            if (v == fl[idx]) {
                                check = false;
                                break;
                            }
                        }

                        if (check) {
                            const Vector3 p(x[v], y[v], z[v]);
                            if (normal.dot(p - center) > Tolerance) {
                                conv = false;
                                break;
                            }
                        }
                    }
                    if (!conv)
                        break;
                    facestart = InvalidIndex;
                }
            }

            if (conv) {
                tl[elem] |= CONVEX_BIT;
            } else {
                ++nonConvexCount;
            }
            break;
        }
        default:
            std::cerr << "invalid element type " << (tl[elem] & TYPE_MASK) << std::endl;
            ++nonConvexCount;
            break;
        }
    }

    return nonConvexCount;
}

std::pair<Vector3, Vector3> UnstructuredGrid::cellBounds(Index elem) const
{
    return elementBounds(elem);
}

Index UnstructuredGrid::findCell(const Vector3 &point, Index hint, int flags) const
{
    const bool acceptGhost = flags & AcceptGhost;
    const bool useCelltree = (flags & ForceCelltree) || (hasCelltree() && !(flags & NoCelltree));

    if (hint != InvalidIndex && inside(hint, point)) {
        return hint;
    }

    const auto bounds = getBounds();
    const auto &min = bounds.first, &max = bounds.second;
    if (point.x() < min.x() || point.y() < min.y() || point.z() < min.z() || point.x() > max.x() ||
        point.y() > max.y() || point.z() > max.z()) {
        return InvalidIndex;
    }

    if (useCelltree) {
        vistle::PointVisitationFunctor<Scalar, Index> nodeFunc(point);
        vistle::PointInclusionFunctor<UnstructuredGrid, Scalar, Index> elemFunc(this, point, acceptGhost);
        getCelltree()->traverse(nodeFunc, elemFunc);
        return elemFunc.cell;
    }

    Index size = getNumElements();
    for (Index i = 0; i < size; ++i) {
        if ((acceptGhost || !isGhostCell(i)) && i != hint) {
            if (inside(i, point)) {
                return i;
            }
        }
    }
    return InvalidIndex;
}

namespace {


// cf. http://stackoverflow.com/questions/808441/inverse-bilinear-interpolation
Vector2 bilinearInverse(const Vector3 &p0, const Vector3 p[4])
{
    const int iter = 5;
    const Scalar tol = 1e-6f;
    Vector2 ss(0.5, 0.5); // initial guess

    const Scalar tol2 = tol * tol;
    for (int k = 0; k < iter; ++k) {
        const Scalar s(ss[0]), t(ss[1]);
        Vector3 res = p[0] * (1 - s) * (1 - t) + p[1] * s * (1 - t) + p[2] * s * t + p[3] * (1 - s) * t - p0;

        //std::cerr << "iter: " << k << ", res: " << res.transpose() << ", weights: " << ss.transpose() << std::endl;

        if (res.squaredNorm() < tol2)
            break;
        const auto Js = -p[0] * (1 - t) + p[1] * (1 - t) + p[2] * t - p[3] * t;
        const auto Jt = -p[0] * (1 - s) - p[1] * s + p[2] * s + p[3] * (1 - s);
        Matrix3x2 J;
        J << Js, Jt;
        ss -= (J.transpose() * J).llt().solve(J.transpose() * res);
    }
    return ss;
}

} // namespace

Scalar UnstructuredGrid::cellDiameter(Index elem) const
{
#if 0
    // compute distance of some vertices which are distant to each other
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];
    auto verts = cellVertices(elem);
    if (verts.empty())
        return 0;
    Index v0 = verts[0];
    Vector3 p(x[v0], y[v0], z[v0]);
    Vector3 farAway(p);
    Scalar dist2(0);
    for (Index v: verts) {
        Vector3 q(x[v], y[v], z[v]);
        Scalar d = (p-q).squaredNorm();
        if (d > dist2) {
			farAway = q;
            dist2 = d;
        }
    }
    for (Index v: verts) {
        Vector3 q(x[v], y[v], z[v]);
        Scalar d = (farAway -q).squaredNorm();
        if (d > dist2) {
            dist2 = d;
        }
    }
    return sqrt(dist2);
#else
    auto bounds = cellBounds(elem);
    return (bounds.second - bounds.first).norm();
#endif
}

Vector3 UnstructuredGrid::cellCenter(Index elem) const
{
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];
    auto verts = cellVertices(elem);
    Vector3 center(0, 0, 0);
    for (auto v: verts) {
        Vector3 p(x[v], y[v], z[v]);
        center += p;
    }
    center *= 1.0f / verts.size();
#ifndef NDEBUG
    const auto bounds = cellBounds(elem);
    auto &min = bounds.first, &max = bounds.second;
    for (int i = 0; i < 3; ++i) {
        assert(center[i] >= min[i]);
        assert(center[i] <= max[i]);
    }
#endif
    return center;
}

std::vector<Index> UnstructuredGrid::getNeighborElements(Index elem) const
{
    return getNeighborFinder().getNeighborElements(elem);
}

Index UnstructuredGrid::cellNumFaces(Index elem) const
{
    auto t = tl()[elem] & TYPE_MASK;
    switch (t) {
    case NONE:
    case BAR:
        return 0;
    case TRIANGLE:
    case QUAD:
    case POLYGON:
        return 1;
    case TETRAHEDRON:
        return 4;
    case PYRAMID:
        return 5;
    case PRISM:
        return 5;
    case HEXAHEDRON:
        return 6;
    case CPOLYHEDRON:
    case VPOLYHEDRON:
        return 0;
    }

    return -1;
}


bool UnstructuredGrid::insideConvex(Index elem, const Vector3 &point) const
{
    //const Scalar Tolerance = 1e-2*cellDiameter(elem); // too slow: halves particle tracing speed
    //const Scalar Tolerance = 1e-5;
    const Scalar Tolerance = 0;

    const Index *el = &this->el()[0];
    const Index *cl = &this->cl()[el[elem]];
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];

    const auto type(tl()[elem] & TYPE_MASK);
    if (type == UnstructuredGrid::VPOLYHEDRON) {
        const Index begin = el[elem], end = el[elem + 1];
        const Index nvert = end - begin;
        for (Index i = 0; i < nvert; i += cl[i] + 1) {
            const auto nc = faceNormalAndCenter(cl[i], &cl[i + 1], x, y, z);
            auto &normal = nc.first;
            auto &center = nc.second;
            if (normal.dot(point - center) > Tolerance)
                return false;
        }
        return true;
    } else if (type == UnstructuredGrid::CPOLYHEDRON) {
        const Index begin = el[elem], end = el[elem + 1];
        const Index nvert = end - begin;
        Index facestart = InvalidIndex;
        Index term = 0;
        for (Index i = 0; i < nvert; ++i) {
            if (facestart == InvalidIndex) {
                term = cl[i];
            } else if (cl[i] == term) {
                const auto nc = faceNormalAndCenter(i - facestart, &cl[facestart], x, y, z);
                auto &normal = nc.first;
                auto &center = nc.second;
                if (normal.dot(point - center) > Tolerance)
                    return false;
                facestart = InvalidIndex;
            }
        }
        return true;
    } else {
        const auto numFaces = NumFaces[type];
        for (int f = 0; f < numFaces; ++f) {
            const auto nc = faceNormalAndCenter(type, f, cl, x, y, z);
            auto &normal = nc.first;
            auto &center = nc.second;

            //std::cerr << "normal: " << n.transpose() << ", v0: " << v0.transpose() << ", rel: " << (point-v0).transpose() << ", dot: " << n.dot(point-v0) << std::endl;

            if (normal.dot(point - center) > Tolerance)
                return false;
        }
        return true;
    }

    return false;
}

Scalar UnstructuredGrid::exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const
{
    const Index *el = &this->el()[0];
    const Index begin = el[elem], end = el[elem + 1];
    const Index *cl = &this->cl()[begin];
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];

    const Vector3 raydir(dir.normalized());

    Scalar exitDist = -1;
    const auto type(tl()[elem] & TYPE_MASK);
    if (type == UnstructuredGrid::VPOLYHEDRON) {
        const Index nvert = end - begin;
        for (Index i = 0; i < nvert; i += cl[i] + 1) {
            const Index nCorners = cl[i];
            auto nc = faceNormalAndCenter(cl[i], &cl[i + 1], x, y, z);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(raydir);
            if (std::abs(cosa) <= Epsilon) {
                continue;
            }
            const Scalar t = normal.dot(center - point) / cosa;
            if (t < 0) {
                continue;
            }
            std::vector<Vector3> corners(nCorners);
            for (Index k = 0; k < nCorners; ++k) {
                const Index v = cl[i + k + 1];
                corners[k] = Vector3(x[v], y[v], z[v]);
            }
            const auto isect = point + t * raydir;
            if (insidePolygon(isect, corners.data(), nCorners, normal)) {
                if (exitDist < 0 || t < exitDist)
                    exitDist = t;
            }
        }
    } else if (type == UnstructuredGrid::CPOLYHEDRON) {
        const Index nvert = end - begin;
        Index term = 0;
        Index facestart = InvalidIndex;
        for (Index i = 0; i < nvert; ++i) {
            if (facestart == InvalidIndex) {
                facestart = i;
                term = cl[i];
            } else if (cl[i] == term) {
                const Index nCorners = i - facestart;
                auto nc = faceNormalAndCenter(nCorners, &cl[facestart], x, y, z);
                auto normal = nc.first;
                auto center = nc.second;

                const Scalar cosa = normal.dot(raydir);
                if (std::abs(cosa) <= Epsilon) {
                    continue;
                }
                const Scalar t = normal.dot(center - point) / cosa;
                if (t < 0) {
                    continue;
                }
                std::vector<Vector3> corners(nCorners);
                for (Index k = 0; k < nCorners; ++k) {
                    const Index v = cl[facestart + k];
                    corners[k] = Vector3(x[v], y[v], z[v]);
                }
                const auto isect = point + t * raydir;
                if (insidePolygon(isect, corners.data(), nCorners, normal)) {
                    if (exitDist < 0 || t < exitDist)
                        exitDist = t;
                }
                facestart = InvalidIndex;
            }
        }
    } else {
        const auto numFaces = NumFaces[type];
        Vector3 corners[4];
        for (int f = 0; f < numFaces; ++f) {
            auto nc = faceNormalAndCenter(type, f, cl, x, y, z);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(raydir);
            if (std::abs(cosa) <= Epsilon) {
                continue;
            }
            const Scalar t = normal.dot(center - point) / cosa;
            if (t < 0) {
                continue;
            }
            const int nCorners = FaceSizes[type][f];
            for (int i = 0; i < nCorners; ++i) {
                const Index v = cl[FaceVertices[type][f][i]];
                corners[i] = Vector3(x[v], y[v], z[v]);
            }
            const auto isect = point + t * raydir;
            if (insidePolygon(isect, corners, nCorners, normal)) {
                if (exitDist < 0 || t < exitDist)
                    exitDist = t;
            }
        }
    }
    return exitDist;
}

bool UnstructuredGrid::inside(Index elem, const Vector3 &point) const
{
    if (elem == InvalidIndex)
        return false;

    if (isConvex(elem))
        return insideConvex(elem, point);

    const auto type = tl()[elem] & TYPE_MASK;
    const Index begin = el()[elem];
    const Index end = el()[elem + 1];
    const Index *cl = &this->cl()[begin];

    return insideCell(point, type, end - begin, cl, &this->x()[0], &this->y()[0], &this->z()[0]);
}

GridInterface::Interpolator UnstructuredGrid::getInterpolator(Index elem, const Vector3 &point, Mapping mapping,
                                                              InterpolationMode mode) const
{
#ifdef INTERPOL_DEBUG
    assert(inside(elem, point));

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
    const Scalar *x[3] = {&this->x()[0], &this->y()[0], &this->z()[0]};

    const Index nvert = el[elem + 1] - el[elem];
    std::vector<Scalar> weights((mode == Linear || mode == Mean) ? nvert : 1);
    std::vector<Index> indices((mode == Linear || mode == Mean) ? nvert : 1);

    if (mode == Mean) {
        if ((tl[elem] & TYPE_MASK) == VPOLYHEDRON || (tl[elem] & TYPE_MASK) == CPOLYHEDRON) {
            indices = cellVertices(elem);
            const Index n = indices.size();
            Scalar w = Scalar(1) / n;
            weights.resize(n, w);
        } else {
            const Scalar w = Scalar(1) / nvert;
            for (Index i = 0; i < nvert; ++i) {
                indices[i] = cl[i];
                weights[i] = w;
            }
        }
    } else if (mode == Linear) {
        switch (tl[elem] & TYPE_MASK) {
        case TETRAHEDRON: {
            assert(nvert == 4);
            Vector3 coord[4];
            for (int i = 0; i < 4; ++i) {
                const Index ind = cl[i];
                indices[i] = ind;
                for (int c = 0; c < 3; ++c) {
                    coord[i][c] = x[c][ind];
                }
            }
            Matrix3 T;
            T << coord[0] - coord[3], coord[1] - coord[3], coord[2] - coord[3];
            Vector3 w = T.inverse() * (point - coord[3]);
            weights[3] = 1.;
            for (int c = 0; c < 3; ++c) {
                weights[c] = w[c];
                weights[3] -= w[c];
            }
            break;
        }
        case PYRAMID: {
            assert(nvert == 5);
            Vector3 coord[5];
            for (int i = 0; i < 5; ++i) {
                const Index ind = cl[i];
                indices[i] = ind;
                for (int c = 0; c < 3; ++c) {
                    coord[i][c] = x[c][ind];
                }
            }
            const Vector3 first(coord[1] - coord[0]);
            Vector3 normal(0, 0, 0);
            for (int i = 2; i < 4; ++i) {
                normal += cross(first, coord[i] - coord[i - 1]);
            }
            const Vector3 top = coord[4] - coord[0];
            const Scalar h = normal.dot(top);
            const Scalar hp = normal.dot(point - coord[0]);
            weights[4] = hp / h;
            const Scalar w = 1 - weights[4];
            const Vector3 p = (point - weights[4] * coord[4]) / w;
            const Vector2 ss = bilinearInverse(p, coord);
            weights[0] = (1 - ss[0]) * (1 - ss[1]) * w;
            weights[1] = ss[0] * (1 - ss[1]) * w;
            weights[2] = ss[0] * ss[1] * w;
            weights[3] = (1 - ss[0]) * ss[1] * w;
            break;
        }
        case PRISM: {
            assert(nvert == 6);
            Vector3 coord[8];
            for (int i = 0; i < 3; ++i) {
                const Index ind1 = cl[i];
                const Index ind2 = cl[i + 3];
                indices[i] = ind1;
                indices[i + 3] = ind2;
                for (int c = 0; c < 3; ++c) {
                    coord[i][c] = x[c][ind1];
                    coord[i + 4][c] = x[c][ind2];
                }
            }
            // we interpolate in a hexahedron with coinciding corners
            coord[3] = coord[2];
            coord[7] = coord[6];
            const Vector3 ss = trilinearInverse(point, coord);
            weights[0] = (1 - ss[0]) * (1 - ss[1]) * (1 - ss[2]);
            weights[1] = ss[0] * (1 - ss[1]) * (1 - ss[2]);
            weights[2] = ss[1] * (1 - ss[2]);
            weights[3] = (1 - ss[0]) * (1 - ss[1]) * ss[2];
            weights[4] = ss[0] * (1 - ss[1]) * ss[2];
            weights[5] = ss[1] * ss[2];
            break;
        }
        case HEXAHEDRON: {
            assert(nvert == 8);
            Vector3 coord[8];
            for (int i = 0; i < 8; ++i) {
                const Index ind = cl[i];
                indices[i] = ind;
                for (int c = 0; c < 3; ++c) {
                    coord[i][c] = x[c][ind];
                }
            }
            const Vector3 ss = trilinearInverse(point, coord);
            weights[0] = (1 - ss[0]) * (1 - ss[1]) * (1 - ss[2]);
            weights[1] = ss[0] * (1 - ss[1]) * (1 - ss[2]);
            weights[2] = ss[0] * ss[1] * (1 - ss[2]);
            weights[3] = (1 - ss[0]) * ss[1] * (1 - ss[2]);
            weights[4] = (1 - ss[0]) * (1 - ss[1]) * ss[2];
            weights[5] = ss[0] * (1 - ss[1]) * ss[2];
            weights[6] = ss[0] * ss[1] * ss[2];
            weights[7] = (1 - ss[0]) * ss[1] * ss[2];
            break;
        }
        case VPOLYHEDRON: {
            /* subdivide n-hedron into n pyramids with tip at the center,
               interpolate within pyramid containing point */

            // polyhedron compute center
            std::vector<Index> verts = cellVertices(elem);
            Vector3 center(0, 0, 0);
            for (auto v: verts) {
                Vector3 c(x[0][v], x[1][v], x[2][v]);
                center += c;
            }
            center /= verts.size();
#ifdef INTERPOL_DEBUG
            std::cerr << "center: " << center.transpose() << std::endl;
            assert(inside(elem, center));
#endif

            std::vector<Vector3> coord(nvert);
            Index nfaces = 0;
            Index n = 0;
            for (Index i = 0; i < nvert; i += cl[i] + 1) {
                const Index N = cl[i];
                ++nfaces;
                for (Index k = i + 1; k < i + N + 1; ++k) {
                    indices[n] = cl[k];
                    for (int c = 0; c < 3; ++c) {
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
            Vector3 isect;
            bool foundFace = false;
            n = 0;
            Index nFaceVert = 0;
            Vector3 faceCenter(0, 0, 0);
            for (Index i = 0; i < nvert; i += cl[i] + 1) {
                nFaceVert = cl[i];
                auto nc = faceNormalAndCenter(nFaceVert, &cl[i + 1], x[0], x[1], x[2]);
                const Vector3 dir = point - center;
                Vector3 normal = nc.first;
                faceCenter = nc.second;
                scale = normal.dot(dir) / normal.dot(faceCenter - center);
#ifdef INTERPOL_DEBUG
                std::cerr << "face: normal=" << normal.transpose() << ", center=" << faceCenter.transpose()
                          << ", endvert=" << i << ", numvert=" << nFaceVert << ", scale=" << scale << std::endl;
                assert(scale <= 1.01); // otherwise, point is outside of the polyhedron
#endif
                if (scale > 1.)
                    scale = 1;
                if (scale >= 0) {
                    scale = std::min(Scalar(1), scale);
                    if (scale > 0.001) {
                        isect = center + dir / scale;
#ifdef INTERPOL_DEBUG
                        std::cerr << "isect: " << isect.transpose() << std::endl;
#endif
                        if (insideConvexPolygon(isect, &coord[n], nFaceVert, normal)) {
#ifdef INTERPOL_DEBUG
                            std::cerr << "found face: normal: " << normal.transpose()
                                      << ", first: " << coord[n].transpose() << ", dir: " << dir.transpose()
                                      << ", isect: " << isect.transpose() << std::endl;
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
                for (Index i = 0; i < ncoord; ++i) {
                    if (!usedIndices.insert(indices[i]).second) {
                        weights[i] = 0;
                        continue;
                    }
                    Scalar centerDist = (coord[i] - center).norm();
#ifdef INTERPOL_DEBUG
                    //std::cerr << "ind " << i << ", centerDist: " << centerDist << std::endl;
#endif
                    if (std::abs(centerDist) > 0) {
                        weights[i] = 1 / centerDist;
                        sum += weights[i];
                    } else {
                        for (Index j = 0; j < ncoord; ++j) {
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
                for (Index i = 0; i < ncoord; ++i) {
                    weights[i] *= centerWeight / sum;
                }
            } else {
                for (Index i = 0; i < ncoord; ++i) {
                    weights[i] = 0;
                }
            }

            if (foundFace) {
                // contribution of hit face,
                // interpolate compatible with simple cells for faces with 3 or 4 vertices
                if (nFaceVert == 3) {
                    Matrix2 T;
                    T << (coord[startIndex + 0] - coord[startIndex + 2]).block<2, 1>(0, 0),
                        (coord[startIndex + 1] - coord[startIndex + 2]).block<2, 1>(0, 0);
                    Vector2 w = T.inverse() * (isect - coord[startIndex + 2]).block<2, 1>(0, 0);
                    weights[startIndex] += w[0] * (1 - centerWeight);
                    weights[startIndex + 1] += w[1] * (1 - centerWeight);
                    weights[startIndex + 2] += (1 - w[0] - w[1]) * (1 - centerWeight);
                } else if (nFaceVert == 4) {
                    Vector2 ss = bilinearInverse(isect, &coord[startIndex]);
                    weights[startIndex] += (1 - ss[0]) * (1 - ss[1]) * (1 - centerWeight);
                    weights[startIndex + 1] += ss[0] * (1 - ss[1]) * (1 - centerWeight);
                    weights[startIndex + 2] += ss[0] * ss[1] * (1 - centerWeight);
                    weights[startIndex + 3] += (1 - ss[0]) * ss[1] * (1 - centerWeight);
                } else if (nFaceVert > 0) {
                    // subdivide face into triangles around faceCenter
                    Scalar sum = 0;
                    std::vector<Scalar> fweights(nFaceVert);
                    for (Index i = 0; i < nFaceVert; ++i) {
                        Scalar centerDist = (coord[i] - faceCenter).norm();
                        fweights[i] = 1 / centerDist;
                        sum += fweights[i];
                    }
                    for (Index i = 0; i < nFaceVert; ++i) {
                        weights[i + startIndex] += fweights[i] / sum * (1 - centerWeight);
                    }
                }
            }
            break;
        }
        case CPOLYHEDRON: {
            /* subdivide n-hedron into n pyramids with tip at the center,
               interpolate within pyramid containing point */

            // polyhedron compute center
            std::vector<Index> verts = cellVertices(elem);
            Vector3 center(0, 0, 0);
            for (auto v: verts) {
                Vector3 c(x[0][v], x[1][v], x[2][v]);
                center += c;
            }
            center /= verts.size();
#ifdef INTERPOL_DEBUG
            std::cerr << "center: " << center.transpose() << std::endl;
            assert(inside(elem, center));
#endif

            std::vector<Vector3> coord(nvert);
            Index nfaces = 0;
            Index n = 0;
            Index facestart = InvalidIndex;
            Index term = 0;
            for (Index i = 0; i < nvert; ++i) {
                if (facestart == InvalidIndex) {
                    facestart = i;
                    term = cl[i];
                } else if (cl[i] == term) {
                    const Index N = i - facestart;
                    ++nfaces;
                    for (Index k = facestart; k < facestart + N; ++k) {
                        indices[n] = cl[k];
                        for (int c = 0; c < 3; ++c) {
                            coord[n][c] = x[c][cl[k]];
                        }
                        ++n;
                    }
                    facestart = InvalidIndex;
                }
            }
            Index ncoord = n;
            coord.resize(ncoord);
            weights.resize(ncoord);

            // find face that is hit by ray from polyhedron center through query point
            Scalar scale = 0;
            Vector3 isect;
            bool foundFace = false;
            n = 0;
            Index nFaceVert = 0;
            Vector3 faceCenter(0, 0, 0);
            facestart = InvalidIndex;
            term = 0;
            for (Index i = 0; i < nvert; ++i) {
                if (facestart == InvalidIndex) {
                    facestart = i;
                    term = cl[i];
                } else if (term == cl[i]) {
                    nFaceVert = i - facestart;
                    auto nc = faceNormalAndCenter(nFaceVert, &cl[facestart], x[0], x[1], x[2]);
                    const Vector3 dir = point - center;
                    Vector3 normal = nc.first;
                    faceCenter = nc.second;
                    scale = normal.dot(dir) / normal.dot(faceCenter - center);
#ifdef INTERPOL_DEBUG
                    std::cerr << "face: normal=" << normal.transpose() << ", center=" << faceCenter.transpose()
                              << ", endvert=" << i << ", numvert=" << nFaceVert << ", scale=" << scale << std::endl;
                    assert(scale <= 1.01); // otherwise, point is outside of the polyhedron
#endif
                    if (scale > 1.)
                        scale = 1;
                    if (scale >= 0) {
                        scale = std::min(Scalar(1), scale);
                        if (scale > 0.001) {
                            isect = center + dir / scale;
#ifdef INTERPOL_DEBUG
                            std::cerr << "isect: " << isect.transpose() << std::endl;
#endif
                            if (insideConvexPolygon(isect, &coord[n], nFaceVert, normal)) {
#ifdef INTERPOL_DEBUG
                                std::cerr << "found face: normal: " << normal.transpose()
                                          << ", first: " << coord[n].transpose() << ", dir: " << dir.transpose()
                                          << ", isect: " << isect.transpose() << std::endl;
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
                    facestart = InvalidIndex;
                }
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
                for (Index i = 0; i < ncoord; ++i) {
                    if (!usedIndices.insert(indices[i]).second) {
                        weights[i] = 0;
                        continue;
                    }
                    Scalar centerDist = (coord[i] - center).norm();
#ifdef INTERPOL_DEBUG
                    //std::cerr << "ind " << i << ", centerDist: " << centerDist << std::endl;
#endif
                    if (std::abs(centerDist) > 0) {
                        weights[i] = 1 / centerDist;
                        sum += weights[i];
                    } else {
                        for (Index j = 0; j < ncoord; ++j) {
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
                for (Index i = 0; i < ncoord; ++i) {
                    weights[i] *= centerWeight / sum;
                }
            } else {
                for (Index i = 0; i < ncoord; ++i) {
                    weights[i] = 0;
                }
            }

            if (foundFace) {
                // contribution of hit face,
                // interpolate compatible with simple cells for faces with 3 or 4 vertices
                if (nFaceVert == 3) {
                    Matrix2 T;
                    T << (coord[startIndex + 0] - coord[startIndex + 2]).block<2, 1>(0, 0),
                        (coord[startIndex + 1] - coord[startIndex + 2]).block<2, 1>(0, 0);
                    Vector2 w = T.inverse() * (isect - coord[startIndex + 2]).block<2, 1>(0, 0);
                    weights[startIndex] += w[0] * (1 - centerWeight);
                    weights[startIndex + 1] += w[1] * (1 - centerWeight);
                    weights[startIndex + 2] += (1 - w[0] - w[1]) * (1 - centerWeight);
                } else if (nFaceVert == 4) {
                    Vector2 ss = bilinearInverse(isect, &coord[startIndex]);
                    weights[startIndex] += (1 - ss[0]) * (1 - ss[1]) * (1 - centerWeight);
                    weights[startIndex + 1] += ss[0] * (1 - ss[1]) * (1 - centerWeight);
                    weights[startIndex + 2] += ss[0] * ss[1] * (1 - centerWeight);
                    weights[startIndex + 3] += (1 - ss[0]) * ss[1] * (1 - centerWeight);
                } else if (nFaceVert > 0) {
                    // subdivide face into triangles around faceCenter
                    Scalar sum = 0;
                    std::vector<Scalar> fweights(nFaceVert);
                    for (Index i = 0; i < nFaceVert; ++i) {
                        Scalar centerDist = (coord[i] - faceCenter).norm();
                        fweights[i] = 1 / centerDist;
                        sum += fweights[i];
                    }
                    for (Index i = 0; i < nFaceVert; ++i) {
                        weights[i + startIndex] += fweights[i] / sum * (1 - centerWeight);
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
        } else if (mode == Nearest) {
            Scalar mindist = std::numeric_limits<Scalar>::max();

            if ((tl[elem] & TYPE_MASK) == VPOLYHEDRON) {
                indices = cellVertices(elem);
                const Index n = indices.size();
                for (Index i = 0; i < n; ++i) {
                    const Index k = indices[i];
                    const Vector3 vert(x[0][k], x[1][k], x[2][k]);
                    const Scalar dist = (point - vert).squaredNorm();
                    if (dist < mindist) {
                        mindist = dist;
                        indices[0] = k;
                    }
                }
            } else {
                // also for CPOLYHEDRON
                for (Index i = 0; i < nvert; ++i) {
                    const Index k = cl[i];
                    const Vector3 vert(x[0][k], x[1][k], x[2][k]);
                    const Scalar dist = (point - vert).squaredNorm();
                    if (dist < mindist) {
                        mindist = dist;
                        indices[0] = k;
                    }
                }
            }
        }
    }

    return Interpolator(weights, indices);
}

std::pair<Vector3, Vector3> UnstructuredGrid::elementBounds(Index elem) const
{
    const auto t = tl()[elem] & UnstructuredGrid::TYPE_MASK;
    if (NumVertices[t] >= 0) {
        return Base::elementBounds(elem);
    }

    if (t == UnstructuredGrid::VPOLYHEDRON) {
        const Index *el = &this->el()[0];
        const Index *cl = &this->cl()[0];
        const Scalar *x[3] = {&this->x()[0], &this->y()[0], &this->z()[0]};
        const Scalar smax = std::numeric_limits<Scalar>::max();
        Vector3 min(smax, smax, smax), max(-smax, -smax, -smax);
        const Index begin = el[elem], end = el[elem + 1];
        for (Index j = begin; j < end; j += cl[j] + 1) {
            Index nvert = cl[j];
            for (Index k = j + 1; k < j + nvert + 1; ++k) {
                Index v = cl[k];
                for (int c = 0; c < 3; ++c) {
                    min[c] = std::min(min[c], x[c][v]);
                    max[c] = std::max(max[c], x[c][v]);
                }
            }
        }
        return std::make_pair(min, max);
    } else if (t == UnstructuredGrid::CPOLYHEDRON) {
        return Base::elementBounds(elem);
    }

    return Base::elementBounds(elem);
}

std::vector<Index> UnstructuredGrid::cellVertices(Index elem) const
{
    const auto t = tl()[elem] & UnstructuredGrid::TYPE_MASK;
    if (NumVertices[t] >= 0) {
        return Base::cellVertices(elem);
    }

    if (t == UnstructuredGrid::VPOLYHEDRON) {
        const Index *el = &this->el()[0];
        const Index *cl = &this->cl()[0];
        const Index begin = el[elem], end = el[elem + 1];
        std::vector<Index> verts;
        verts.reserve(end - begin);
        for (Index j = begin; j < end; j += cl[j] + 1) {
            Index nvert = cl[j];
            for (Index k = j + 1; k < j + nvert + 1; ++k) {
                verts.push_back(cl[k]);
            }
        }
        std::sort(verts.begin(), verts.end());
        auto last = std::unique(verts.begin(), verts.end());
        verts.resize(last - verts.begin());
        return verts;
    } else if (t == UnstructuredGrid::CPOLYHEDRON) {
        const Index *el = &this->el()[0];
        const Index *cl = &this->cl()[0];
        const Index begin = el[elem], end = el[elem + 1];
        std::vector<Index> verts;
        verts.reserve(end - begin);
        for (Index j = begin; j < end; ++j) {
            verts.push_back(cl[j]);
        }
        std::sort(verts.begin(), verts.end());
        auto last = std::unique(verts.begin(), verts.end());
        verts.resize(last - verts.begin());
        return verts;
    }

    return std::vector<Index>();
}

void UnstructuredGrid::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);
    m_tl = (d && d->tl.valid()) ? d->tl->data() : nullptr;
}

void UnstructuredGrid::Data::initData()
{}

UnstructuredGrid::Data::Data(const UnstructuredGrid::Data &o, const std::string &n)
: UnstructuredGrid::Base::Data(o, n), tl(o.tl)
{
    initData();
}

UnstructuredGrid::Data::Data(const Index numElements, const Index numCorners, const Index numVertices,
                             const std::string &name, const Meta &meta)
: UnstructuredGrid::Base::Data(numElements, numCorners, numVertices, Object::UNSTRUCTUREDGRID, name, meta)
{
    initData();
    tl.construct(numElements);
}

UnstructuredGrid::Data *UnstructuredGrid::Data::create(const Index numElements, const Index numCorners,
                                                       const Index numVertices, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *u = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
    publish(u);

    return u;
}

V_OBJECT_TYPE(UnstructuredGrid, Object::UNSTRUCTUREDGRID)
V_OBJECT_CTOR(UnstructuredGrid)
V_OBJECT_IMPL(UnstructuredGrid)

} // namespace vistle
