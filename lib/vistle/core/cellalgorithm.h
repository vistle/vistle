#ifndef VISTLE_CORE_CELLALGORITHM_H
#define VISTLE_CORE_CELLALGORITHM_H

#include "export.h"
#include "vectortypes.h"
#include "celltree.h"
#include "grid.h"

namespace vistle {

V_COREEXPORT Vector3 trilinearInverse(const Vector3 &p0, const Vector3 p[8]);
V_COREEXPORT bool originInsidePolygonZ2D(const Vector3 *corners, Index nCorners);
V_COREEXPORT bool insidePolygon(const Vector3 &point, const Vector3 *corners, Index nCorners, const Vector3 &normal);
V_COREEXPORT bool insideConvexPolygon(const Vector3 &point, const Vector3 *corners, Index nCorners,
                                      const Vector3 &normal);
V_COREEXPORT std::pair<Vector3, Vector3> faceNormalAndCenter(Byte type, Index f, const Vector3 *corners);
V_COREEXPORT std::pair<Vector3, Vector3> faceNormalAndCenter(Index nCorners, const Vector3 *corners);
V_COREEXPORT std::pair<Vector3, Vector3> faceNormalAndCenter(Index nVert, const Index *verts, const Scalar *x,
                                                             const Scalar *y, const Scalar *z);
V_COREEXPORT std::pair<Vector3, Vector3> faceNormalAndCenter(Byte type, Index f, const Index *cl, const Scalar *x,
                                                             const Scalar *y, const Scalar *z);
bool V_COREEXPORT insideCell(const Vector3 &point, Byte type, Index nverts, const Index *cl, const Scalar *x,
                             const Scalar *y, const Scalar *z);

template<typename Scalar, typename Index>
class PointVisitationFunctor: public Celltree<Scalar, Index>::VisitFunctor {
    typedef vistle::Celltree<Scalar, Index> Celltree;
    typedef typename Celltree::VisitFunctor VisitFunctor;
    typedef typename VisitFunctor::Order Order;

public:
    PointVisitationFunctor(const Vector3 &point): m_point(point) {}

    bool checkBounds(const Scalar *min, const Scalar *max)
    {
#ifdef CT_DEBUG
        std::cerr << "checkBounds: min: " << min[0] << " " << min[1] << " " << min[2] << ", max: " << max[0] << " "
                  << max[1] << " " << max[2] << std::endl;
#endif
        for (int i = 0; i < 3; ++i) {
            if (min[i] > m_point[i])
                return false;
            if (max[i] < m_point[i])
                return false;
        }
        return true;
    }

    Order operator()(const typename Celltree::Node &node)
    {
#ifdef CT_DEBUG
        std::cerr << "visit subtree: Lmax: " << node.Lmax << ", Rmin: " << node.Rmin << std::endl;
#endif

        const Scalar c = m_point[node.dim];
        if (c > node.Lmax && c < node.Rmin)
            return VisitFunctor::None;
        if (c < node.Rmin) {
            return VisitFunctor::Left;
        }
        if (c > node.Lmax) {
            return VisitFunctor::Right;
        }

        const Scalar mean = Scalar(0.5) * (node.Lmax + node.Rmin);
        if (c < mean) {
            return VisitFunctor::LeftRight;
        } else {
            return VisitFunctor::RightLeft;
        }
    }

private:
    Vector3 m_point;
};

template<typename Scalar, typename Index>
class LineVisitationFunctor: public Celltree<Scalar, Index>::VisitFunctor {
    typedef vistle::Celltree<Scalar, Index> Celltree;
    typedef typename Celltree::VisitFunctor VisitFunctor;
    typedef typename VisitFunctor::Order Order;

public:
    LineVisitationFunctor(const Vector3 &p0, const Vector3 &p1): m_p0(p0), m_p1(p1) {}

    bool checkBounds(const Scalar *min, const Scalar *max)
    {
#ifdef CT_DEBUG
        std::cerr << "checkBounds: min: " << min[0] << " " << min[1] << " " << min[2] << ", max: " << max[0] << " "
                  << max[1] << " " << max[2] << std::endl;
#endif
        for (int i = 0; i < 3; ++i) {
            if (min[i] > m_p0[i] && min[i] > m_p1[i])
                return false;
            if (max[i] < m_p0[i] && max[i] < m_p1[i])
                return false;
        }
        return true;
    }

    Order operator()(const typename Celltree::Node &node)
    {
#ifdef CT_DEBUG
        std::cerr << "visit subtree: Lmax: " << node.Lmax << ", Rmin: " << node.Rmin << std::endl;
#endif

        const Scalar c0 = m_p0[node.dim];
        const Scalar c1 = m_p1[node.dim];
        if (c0 > node.Lmax && c0 < node.Rmin && c1 > node.Lmax && c1 < node.Rmin)
            return VisitFunctor::None;
        if (c0 < node.Rmin && c1 < node.Rmin) {
            return VisitFunctor::Left;
        }
        if (c0 > node.Lmax && c1 > node.Lmax) {
            return VisitFunctor::Right;
        }

        const Scalar mean = Scalar(0.5) * (node.Lmax + node.Rmin);
        const Scalar c = Scalar(0.5) * (c0 + c1);
        if (c < mean) {
            return VisitFunctor::LeftRight;
        } else {
            return VisitFunctor::RightLeft;
        }
    }

private:
    Vector3 m_p0, m_p1;
};

template<typename Scalar, typename Index>
struct CellBoundsFunctor: public Celltree<Scalar, Index>::CellBoundsFunctor {
    CellBoundsFunctor(const GridInterface *grid): m_grid(grid) {}

    bool operator()(Index elem, Vector3 *min, Vector3 *max) const
    {
        auto b = m_grid->cellBounds(elem);
        *min = b.first;
        *max = b.second;
        return true;
    }

    const GridInterface *m_grid;
};


template<class Grid, typename Scalar, typename Index>
class PointInclusionFunctor: public Celltree<Scalar, Index>::LeafFunctor {
public:
    PointInclusionFunctor(const Grid *grid, const Vector3 &point, bool acceptGhost = false)
    : m_grid(grid), m_point(point), m_acceptGhost(acceptGhost), cell(InvalidIndex)
    {}

    bool operator()(Index elem)
    {
#ifdef CT_DEBUG
        std::cerr << "PointInclusionFunctor: checking cell: " << elem << std::endl;
#endif
        if (m_acceptGhost || !m_grid->isGhostCell(elem)) {
            if (m_grid->inside(elem, m_point)) {
#ifdef CT_DEBUG
                std::cerr << "PointInclusionFunctor: found cell: " << elem << std::endl;
#endif
                cell = elem;
                return false; // stop traversal
            }
        }
        return true;
    }
    const Grid *m_grid;
    Vector3 m_point;
    bool m_acceptGhost;
    Index cell;
};

template<class Grid, typename Scalar, typename Index>
class LineIntersectionFunctor: public Celltree<Scalar, Index>::LeafFunctor {
public:
    LineIntersectionFunctor(const Grid *grid, const Vector3 &p0, const Vector3 &p1, bool acceptGhost = false)
    : m_grid(grid), m_p0(p0), m_dir(p1 - p0), m_acceptGhost(acceptGhost)
    {}

    bool operator()(Index elem)
    {
#ifdef CT_DEBUG
        std::cerr << "LineIntersectionFunctor: checking cell: " << elem << std::endl;
#endif
        if (m_acceptGhost || !m_grid->isGhostCell(elem)) {
            if (m_grid->cellNumFaces() != 1)
                return true;
            auto verts = m_grid->cellVertices(elem);
            Index nCorners = verts.size();
            std::vector<Vector3> corners;
            corners.reserve(nCorners);
            for (auto v: verts) {
                corners.emplace_back(m_grid->getVertex(v));
            }
            auto nc = faceNormalAndCenter(corners.size(), &corners[0]);
            auto normal = nc.first;
            auto center = nc.second;

            const Scalar cosa = normal.dot(m_dir);
            if (std::abs(cosa) <= 1e-7) {
                return true;
            }
            const Scalar t = normal.dot(center - m_p0) / cosa;
            if (t < 0 || t > 1) {
                return true;
            }
            const auto isect = m_p0 + t * m_dir;
            if (insidePolygon(isect, corners.data(), nCorners, normal)) {
                intersections.emplace_back(isect, elem);
            }
        }
        return true;
    }
    const Grid *m_grid;
    const Vector3 m_p0;
    const Vector3 m_dir;
    const bool m_acceptGhost;
    struct Intersection {
        Vector3 point;
        Index cell;
    };
    std::vector<Intersection> intersections;
};

} // namespace vistle
#endif
