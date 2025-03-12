#include "unstr.h"
#include "unstr_geo.h"
#include "unstr_impl.h"
#include "archives.h"
#include <cassert>
#include <algorithm>
#include "cellalgorithm.h"
#include <set>
#include "validate.h"

//#define INTERPOL_DEBUG
//#define INSIDE_DEBUG

namespace vistle {

static const Scalar Epsilon = 1e-7;

template<std::size_t N>
struct num {
    static const constexpr auto value = N;
};

template<class F, std::size_t... Is>
void for_(F func, std::index_sequence<Is...>)
{
    (func(num<Is>{}), ...);
}

constexpr std::array<const char *, UnstructuredGrid::NUM_TYPES> TypeNames = {
    "NONE", "POINT",       "",           "BAR",        "POLYLINE", "TRIANGLE", "", "POLYGON", "",
    "QUAD", "TETRAHEDRON", "POLYHEDRON", "HEXAHEDRON", "PRISM",    "PYRAMID"};

constexpr std::array<const char *, UnstructuredGrid::NUM_TYPES> TypeNameAbbreviations = {
    "NONE", "PT", "", "BAR", "LINE", "TRI", "", "PLG", "", "QUAD", "TETRA", "PLH", "HEXA", "PRISM", "PYR"};

constexpr int NumSupportedTypes = 11;
constexpr std::array<UnstructuredGrid::Type, NumSupportedTypes> SupportedTypes = {
    UnstructuredGrid::POINT,      UnstructuredGrid::BAR,   UnstructuredGrid::POLYLINE,    UnstructuredGrid::TRIANGLE,
    UnstructuredGrid::POLYGON,    UnstructuredGrid::QUAD,  UnstructuredGrid::TETRAHEDRON, UnstructuredGrid::POLYHEDRON,
    UnstructuredGrid::HEXAHEDRON, UnstructuredGrid::PRISM, UnstructuredGrid::PYRAMID};

const char *UnstructuredGrid::toString(Type t, bool abbreviation)
{
    assert(t > 0 && t < NUM_TYPES);
    return abbreviation ? TypeNameAbbreviations[t] : TypeNames[t];
}

UnstructuredGrid::UnstructuredGrid(const size_t numElements, const size_t numCorners, const size_t numVertices,
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

    refreshImpl();
}

bool UnstructuredGrid::isEmpty()
{
    return Base::isEmpty();
}

bool UnstructuredGrid::isEmpty() const
{
    return Base::isEmpty();
}

bool UnstructuredGrid::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_INDEX(d()->tl->size());

    VALIDATE(d()->tl->check(os));
    VALIDATE(d()->tl->size() == getNumElements());

    if (quick)
        return true;

    VALIDATE_RANGE_P(d()->tl, 0, cell::NUM_TYPES - 1);

    return true;
}

void UnstructuredGrid::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
    os << " tl:";
    d()->tl.print(os, verbose);
}

bool UnstructuredGrid::isGhostCell(const Index elem) const
{
    if (elem == InvalidIndex)
        return false;
    return isGhost(elem);
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
    auto center = Base::cellCenter(elem);
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

Index UnstructuredGrid::cellNumVertices(Index elem) const
{
    auto t = tl()[elem];
    if (t == POLYHEDRON) {
        const Index *el = &this->el()[0];
        const Index *cl = &this->cl()[0];
        const Index begin = el[elem], end = el[elem + 1];
        std::vector<Index> verts(&cl[begin], &cl[end]);
        std::sort(verts.begin(), verts.end());
        auto last = std::unique(verts.begin(), verts.end());
        return last - verts.begin();
    } else if (t < NUM_TYPES) {
        if (NumVertices[t] < 0) {
            const Index *el = &this->el()[0];
            const Index begin = el[elem], end = el[elem + 1];
            return end - begin;
        }
        return NumVertices[t];
    }
    return 0;
}

Scalar UnstructuredGrid::cellEdgeLength(Index elem) const
{
    Scalar retval = -1;
    auto type = tl()[elem];
    for_<NumSupportedTypes>([&](auto i) {
        if (SupportedTypes[i.value] == type) {
            retval = edgeLength<SupportedTypes[i.value]>(cellNumVertices(elem), &cl()[el()[elem]],
                                                         {&x()[0], &y()[0], &z()[0]});
        }
    });
    return retval;
}

Scalar UnstructuredGrid::cellSurface(Index elem) const
{
    Scalar retval = -1;
    auto type = tl()[elem];
    for_<NumSupportedTypes>([&](auto i) {
        if (SupportedTypes[i.value] == type) {
            retval =
                surface<SupportedTypes[i.value]>(cellNumVertices(elem), &cl()[el()[elem]], {&x()[0], &y()[0], &z()[0]});
            return;
        }
    });
    return retval;
}

Scalar UnstructuredGrid::cellVolume(Index elem) const
{
    Scalar retval = -1;
    auto type = tl()[elem];
    for_<NumSupportedTypes>([&](auto i) {
        if (SupportedTypes[i.value] == type) {
            retval =
                volume<SupportedTypes[i.value]>(cellNumVertices(elem), &cl()[el()[elem]], {&x()[0], &y()[0], &z()[0]});
            return;
        }
    });
    return retval;
}

Index UnstructuredGrid::cellNumFaces(Index elem) const
{
    auto t = tl()[elem];
    if (t == POLYHEDRON) {
        const Index *el = &this->el()[0];
        const Index begin = el[elem], end = el[elem + 1];
        const Index *cl = &this->cl()[begin];
        const Index nverts = end - begin;

        Index numFaces = 0;
        Index term = InvalidIndex;
        for (Index i = 0; i < nverts; ++i) {
            if (term == InvalidIndex) {
                term = cl[i];
            } else if (cl[i] == term) {
                ++numFaces;
                term = InvalidIndex;
            }
        }
        assert(term == InvalidIndex);

        return numFaces;
    } else if (t < NUM_TYPES) {
        return NumFaces[t] >= 0 ? NumFaces[t] : 0;
    }
    return 0;
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
    const auto type(tl()[elem]);
    if (type == UnstructuredGrid::POLYHEDRON) {
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
                std::vector<Vector3> corners;
                corners.reserve(nCorners);
                for (Index k = 0; k < nCorners; ++k) {
                    const Index v = cl[facestart + k];
                    corners.emplace_back(x[v], y[v], z[v]);
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

    const auto type = tl()[elem];
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
        if (tl[elem] == POLYHEDRON) {
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
        switch (tl[elem]) {
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
        case POLYHEDRON: {
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

            std::vector<Vector3> coord;
            coord.reserve(nvert);
            Index n = 0;
            Index facestart = InvalidIndex;
            Index term = 0;
            for (Index i = 0; i < nvert; ++i) {
                if (facestart == InvalidIndex) {
                    facestart = i;
                    term = cl[i];
                } else if (cl[i] == term) {
                    const Index N = i - facestart;
                    for (Index k = facestart; k < facestart + N; ++k) {
                        const auto clk = cl[k];
                        indices[n] = clk;
                        coord.emplace_back(x[0][clk], x[1][clk], x[2][clk]);
                        ++n;
                    }
                    facestart = InvalidIndex;
                }
            }
            Index ncoord = n;
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

            // also for POLYHEDRON
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

    return Interpolator(weights, indices);
}

std::vector<Index> UnstructuredGrid::cellVertices(Index elem) const
{
    const auto t = tl()[elem];

    if (t == UnstructuredGrid::POLYHEDRON) {
        const Index *el = &this->el()[0];
        const Index *cl = &this->cl()[0];
        const Index begin = el[elem], end = el[elem + 1];
        std::vector<Index> verts(&cl[begin], &cl[end]);
        std::sort(verts.begin(), verts.end());
        auto last = std::unique(verts.begin(), verts.end());
        verts.resize(last - verts.begin());
        return verts;
    }

    if (NumVertices[t] >= 0) {
        return Base::cellVertices(elem);
    }

    return std::vector<Index>();
}

void UnstructuredGrid::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);
    if (d) {
        m_tl = d->tl;
    } else {
        m_tl = nullptr;
    }
}

void UnstructuredGrid::Data::initData()
{}

UnstructuredGrid::Data::Data(const UnstructuredGrid::Data &o, const std::string &n)
: UnstructuredGrid::Base::Data(o, n), tl(o.tl)
{
    initData();
}

UnstructuredGrid::Data::Data(const size_t numElements, const size_t numCorners, const size_t numVertices,
                             const std::string &name, const Meta &meta)
: UnstructuredGrid::Base::Data(numElements, numCorners, numVertices, Object::UNSTRUCTUREDGRID, name, meta)
{
    initData();
    tl.construct(numElements);
}

UnstructuredGrid::Data *UnstructuredGrid::Data::create(const size_t numElements, const size_t numCorners,
                                                       const size_t numVertices, const Meta &meta)
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
