//-------------------------------------------------------------------------
// STRUCTURED GRID CLASS H
// *
// * Structured Grid Container Object
//-------------------------------------------------------------------------

#include "structuredgrid.h"
#include "structuredgrid_impl.h"
#include "archives.h"
#include "celltree_impl.h"
#include "unstr.h" // for hexahedron topology
#include <cassert>
#include "cellalgorithm.h"

//#define INTERPOL_DEBUG

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
StructuredGrid::StructuredGrid(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const Meta &meta)
: StructuredGrid::Base(StructuredGrid::Data::create(numVert_x, numVert_y, numVert_z, meta))
{
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void StructuredGrid::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);

    for (int c = 0; c < 3; ++c) {
        if (d && d->x[c].valid()) {
            CHECK_OVERFLOW(d->x[c]->size());

            m_numDivisions[c] = d->numDivisions[c];

            m_ghostLayers[c][0] = d->ghostLayers[c][0];
            m_ghostLayers[c][1] = d->ghostLayers[c][1];
        } else {
            m_numDivisions[c] = 0;

            m_ghostLayers[c][0] = 0;
            m_ghostLayers[c][1] = 0;
        }
    }
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool StructuredGrid::checkImpl() const
{
    V_CHECK(getSize() == getNumDivisions(0) * getNumDivisions(1) * getNumDivisions(2));

    for (int c = 0; c < 3; c++) {
        V_CHECK(d()->ghostLayers[c][0] + d()->ghostLayers[c][1] < getNumDivisions(c));
    }

    return true;
}

void StructuredGrid::updateInternals()
{
    Base::applyDimensionHint(shared_from_this());
    Base::updateInternals();
}

// IS EMPTY
//-------------------------------------------------------------------------
bool StructuredGrid::isEmpty()
{
    return Base::isEmpty();
}

bool StructuredGrid::isEmpty() const
{
    return Base::isEmpty();
}

void StructuredGrid::print(std::ostream &os) const
{
    Base::print(os);
    os << " " << getSize() << "=" << getNumDivisions(0) << "x" << getNumDivisions(1) << "x" << getNumDivisions(2);
}

// GET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
Index StructuredGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos)
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return d()->ghostLayers[dim][layerPosition];
}

// GET FUNCTION - GHOST CELL LAYER CONST
//-------------------------------------------------------------------------
Index StructuredGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return m_ghostLayers[dim][layerPosition];
}

void StructuredGrid::setGlobalIndexOffset(int c, Index offset)
{
    d()->indexOffset[c] = offset;
}

// SET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
void StructuredGrid::setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value)
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    d()->ghostLayers[dim][layerPosition] = value;
    m_ghostLayers[dim][layerPosition] = value;

    return;
}

// HAS CELL TREE CHECK
//-------------------------------------------------------------------------
bool StructuredGrid::hasCelltree() const
{
    if (m_celltree)
        return true;

    return hasAttachment("celltree");
}

// GET FUNCTION - CELL TREE
//-------------------------------------------------------------------------
StructuredGrid::Celltree::const_ptr StructuredGrid::getCelltree() const
{
    if (m_celltree)
        return m_celltree;
    Data::attachment_mutex_lock_type lock(d()->attachment_mutex);
    if (!hasAttachment("celltree")) {
        refresh();
        createCelltree(m_numDivisions);
    }

    m_celltree = Celltree::as(getAttachment("celltree"));
    assert(m_celltree);
    return m_celltree;
}

// VALIDATE CELL TREE CHECK
//-------------------------------------------------------------------------
bool StructuredGrid::validateCelltree() const
{
    if (!hasCelltree())
        return false;

    CellBoundsFunctor<Scalar, Index> boundFunc(this);
    auto ct = getCelltree();
    if (!ct->validateTree(boundFunc)) {
        std::cerr << "StructuredGrid: Celltree validation failed with " << getNumElements()
                  << " elements total, bounds: " << getBounds().first << "-" << getBounds().second << std::endl;
        return false;
    }
    return true;
}

// CREATE CELL TREE
//-------------------------------------------------------------------------
void StructuredGrid::createCelltree(Index dims[3]) const
{
    if (hasCelltree())
        return;

    const Scalar *coords[3] = {&x()[0], &y()[0], &z()[0]};
    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 vmin, vmax;
    vmin.fill(-smax);
    vmax.fill(smax);

    const Index nelem = getNumElements();
    std::vector<Vector3> min(nelem, vmax);
    std::vector<Vector3> max(nelem, vmin);

    Vector3 gmin = vmax, gmax = vmin;
    for (Index el = 0; el < nelem; ++el) {
        const auto corners = cellVertices(el, dims);
        for (int d = 0; d < 3; ++d) {
            for (const auto v: corners) {
                if (min[el][d] > coords[d][v]) {
                    min[el][d] = coords[d][v];
                    if (gmin[d] > min[el][d])
                        gmin[d] = min[el][d];
                }
                if (max[el][d] < coords[d][v]) {
                    max[el][d] = coords[d][v];
                    if (gmax[d] < max[el][d])
                        gmax[d] = max[el][d];
                }
            }
        }
    }

    typename Celltree::ptr ct(new Celltree(nelem));
    ct->init(min.data(), max.data(), gmin, gmax);
    addAttachment("celltree", ct);
#ifndef NDEBUG
    if (!validateCelltree()) {
        std::cerr << "ERROR: Celltree validation failed." << std::endl;
    }
#endif
}

Index StructuredGrid::getNumVertices() const
{
    return getNumDivisions(0) * getNumDivisions(1) * getNumDivisions(2);
}

// GET FUNCTION - BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector3, Vector3> StructuredGrid::getBounds() const
{
    if (hasCelltree()) {
        const auto ct = getCelltree();
        return std::make_pair(Vector3(ct->min()), Vector3(ct->max()));
    }

    return Base::getMinMax();
}

Normals::const_ptr StructuredGrid::normals() const
{
    return Normals::as(d()->normals.getObject());
}

void StructuredGrid::setNormals(Normals::const_ptr normals)
{
    assert(!normals || normals->check());
    d()->normals = normals;
}

// CELL BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector3, Vector3> StructuredGrid::cellBounds(Index elem) const
{
    const Scalar *x[3] = {&this->x()[0], &this->y()[0], &this->z()[0]};
    auto cl = cellVertices(elem, m_numDivisions);

    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 min(smax, smax, smax), max(-smax, -smax, -smax);
    for (Index v: cl) {
        for (int c = 0; c < 3; ++c) {
            min[c] = std::min(min[c], x[c][v]);
            max[c] = std::max(max[c], x[c][v]);
        }
    }
    return std::make_pair(min, max);
}

// FIND CELL
//-------------------------------------------------------------------------
Index StructuredGrid::findCell(const Vector3 &point, Index hint, int flags) const
{
    const bool acceptGhost = flags & AcceptGhost;
    const bool useCelltree = (flags & ForceCelltree) || (hasCelltree() && !(flags & NoCelltree));

    if (hint != InvalidIndex && inside(hint, point)) {
        return hint;
    }

    if (useCelltree) {
        vistle::PointVisitationFunctor<Scalar, Index> nodeFunc(point);
        vistle::PointInclusionFunctor<StructuredGrid, Scalar, Index> elemFunc(this, point, acceptGhost);
        getCelltree()->traverse(nodeFunc, elemFunc);
        return elemFunc.cell;
    }

    Index size = getNumElements();
    for (Index i = 0; i < size; ++i) {
        if ((acceptGhost || !isGhostCell(i)) && i != hint) {
            if (inside(i, point))
                return i;
        }
    }
    return InvalidIndex;
}

// INSIDE CHECK
//-------------------------------------------------------------------------
bool StructuredGrid::inside(Index elem, const Vector3 &point) const
{
    if (elem == InvalidIndex)
        return false;

    const UnstructuredGrid::Type type = UnstructuredGrid::HEXAHEDRON;
    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];

    auto cl = cellVertices(elem, m_numDivisions);

#ifdef ASSUME_CONVEX
    Vector3 corners[8];
    for (int i = 0; i < 8; ++i) {
        corners[i][0] = x[cl[i]];
        corners[i][1] = y[cl[i]];
        corners[i][2] = z[cl[i]];
    }

    const auto numFaces = UnstructuredGrid::NumFaces[type];
    const auto &faces = UnstructuredGrid::FaceVertices[type];
    const auto &sizes = UnstructuredGrid::FaceSizes[type];
    for (int f = 0; f < numFaces; ++f) {
        Vector3 v0 = corners[faces[f][0]];
        Vector3 edge1 = corners[faces[f][1]];
        edge1 -= v0;
        Vector3 n(0, 0, 0);
        for (unsigned i = 2; i < sizes[f]; ++i) {
            Vector3 edge = corners[faces[f][i]];
            edge -= v0;
            n += cross(edge1, edge);
        }

        //std::cerr << "normal: " << n.transpose() << ", v0: " << v0.transpose() << ", rel: " << (point-v0).transpose() << ", dot: " << n.dot(point-v0) << std::endl;

        if (n.dot(point - v0) > 0)
            return false;
    }
    return true;
#else
    return insideCell(point, type, cl.size(), cl.data(), x, y, z);
#endif
}

Scalar StructuredGrid::exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const
{
    refresh();
    auto cl = cellVertices(elem, m_numDivisions);

    const Scalar *x = &this->x()[0];
    const Scalar *y = &this->y()[0];
    const Scalar *z = &this->z()[0];

    const Vector3 raydir(dir.normalized());

    Scalar exitDist = -1;
    const UnstructuredGrid::Type type = UnstructuredGrid::HEXAHEDRON;
    const auto numFaces = UnstructuredGrid::NumFaces[type];
    const auto &faces = UnstructuredGrid::FaceVertices[type];
    const auto &sizes = UnstructuredGrid::FaceSizes[type];
    Vector3 corners[4];
    for (int f = 0; f < numFaces; ++f) {
        auto nc = faceNormalAndCenter(type, f, cl.data(), x, y, z);
        auto normal = nc.first;
        auto center = nc.second;

        const Scalar cosa = normal.dot(raydir);
        if (std::abs(cosa) <= 1e-7) {
            continue;
        }
        const Scalar t = normal.dot(center - point) / cosa;
        if (t < 0) {
            continue;
        }
        const int nCorners = sizes[f];
        for (int i = 0; i < nCorners; ++i) {
            const Index v = cl[faces[f][i]];
            corners[i] = Vector3(x[v], y[v], z[v]);
        }
        const auto isect = point + t * raydir;
        if (insidePolygon(isect, corners, nCorners, normal)) {
            if (exitDist < 0 || t < exitDist)
                exitDist = t;
        }
    }
    return exitDist;
}

void StructuredGrid::copyAttributes(Object::const_ptr src, bool replace)
{
    Base::copyAttributes(src, replace);
    if (auto s = StructuredGridBase::as(src)) {
        for (int c = 0; c < 3; ++c)
            setGlobalIndexOffset(c, s->getGlobalIndexOffset(c));
    }
}

// GET INTERPOLATOR
//-------------------------------------------------------------------------
GridInterface::Interpolator StructuredGrid::getInterpolator(Index elem, const Vector3 &point, DataBase::Mapping mapping,
                                                            GridInterface::InterpolationMode mode) const
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

    auto cl = cellVertices(elem, m_numDivisions);
    Index nvert = cl.size();

    const Scalar *x[3] = {&this->x()[0], &this->y()[0], &this->z()[0]};
    std::vector<Vector3> corners(nvert);
    for (Index i = 0; i < nvert; ++i) {
        corners[i][0] = x[0][cl[i]];
        corners[i][1] = x[1][cl[i]];
        corners[i][2] = x[2][cl[i]];
    }

    std::vector<Index> indices((mode == Linear || mode == Mean) ? nvert : 1);
    std::vector<Scalar> weights((mode == Linear || mode == Mean) ? nvert : 1);

    if (mode == Mean) {
        const Scalar w = Scalar(1) / nvert;
        for (Index i = 0; i < nvert; ++i) {
            indices[i] = cl[i];
            weights[i] = w;
        }
    } else if (mode == Linear && nvert == 8) {
        assert(nvert == 8);
        for (Index i = 0; i < nvert; ++i) {
            indices[i] = cl[i];
        }
        const Vector3 ss = trilinearInverse(point, corners.data());
        weights[0] = (1 - ss[0]) * (1 - ss[1]) * (1 - ss[2]);
        weights[1] = ss[0] * (1 - ss[1]) * (1 - ss[2]);
        weights[2] = ss[0] * ss[1] * (1 - ss[2]);
        weights[3] = (1 - ss[0]) * ss[1] * (1 - ss[2]);
        weights[4] = (1 - ss[0]) * (1 - ss[1]) * ss[2];
        weights[5] = ss[0] * (1 - ss[1]) * ss[2];
        weights[6] = ss[0] * ss[1] * ss[2];
        weights[7] = (1 - ss[0]) * ss[1] * ss[2];
    } else {
        weights[0] = 1;

        if (mode == First) {
            indices[0] = cl[0];
        } else if (mode == Nearest) {
            Scalar mindist = std::numeric_limits<Scalar>::max();

            for (Index i = 0; i < nvert; ++i) {
                const Index k = cl[i];
                const Vector3 vert(x[0][k], x[1][k], x[2][k]);
                const Scalar dist = (point - vert).squaredNorm();
                if (dist < mindist) {
                    indices[0] = k;
                    mindist = dist;
                }
            }
        }
    }

    return Interpolator(weights, indices);
}

void StructuredGrid::Data::initData()
{
    for (int i = 0; i < 3; ++i) {
        indexOffset[i] = 0;
        numDivisions[i] = 0;
        ghostLayers[i][0] = ghostLayers[i][1] = 0;
    }
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
StructuredGrid::Data::Data(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z,
                           const std::string &name, const Meta &meta)
: StructuredGrid::Base::Data(numVert_x * numVert_y * numVert_z, Object::STRUCTUREDGRID, name, meta)
{
    CHECK_OVERFLOW(numVert_x);
    CHECK_OVERFLOW(numVert_y);
    CHECK_OVERFLOW(numVert_z);
    CHECK_OVERFLOW(numVert_x * numVert_y * numVert_z);

    initData();

    numDivisions[0] = numVert_x;
    numDivisions[1] = numVert_y;
    numDivisions[2] = numVert_z;

    x[0]->setDimensionHint(numVert_x, numVert_y, numVert_z);
    x[1]->setDimensionHint(numVert_x, numVert_y, numVert_z);
    x[2]->setDimensionHint(numVert_x, numVert_y, numVert_z);
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
StructuredGrid::Data::Data(const StructuredGrid::Data &o, const std::string &n): StructuredGrid::Base::Data(o, n)
{
    initData();

    normals = o.normals;
    for (int c = 0; c < 3; ++c) {
        indexOffset[c] = o.indexOffset[c];
        numDivisions[c] = o.numDivisions[c];
        ghostLayers[c][0] = o.ghostLayers[c][0];
        ghostLayers[c][1] = o.ghostLayers[c][1];
    }
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
StructuredGrid::Data::~Data()
{}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
StructuredGrid::Data *StructuredGrid::Data::create(const size_t numVert_x, const size_t numVert_y,
                                                   const size_t numVert_z, const Meta &meta)
{
    // construct shm data
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numVert_x, numVert_y, numVert_z, name, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(StructuredGrid, Object::STRUCTUREDGRID)
V_OBJECT_CTOR(StructuredGrid)
V_OBJECT_IMPL(StructuredGrid)

} // namespace vistle
