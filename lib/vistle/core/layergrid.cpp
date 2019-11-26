#include "object.h"
#include "unstr.h" // for hexahedron topology
#include "archives.h"
#include <cassert>
#include "cellalgorithm.h"
#include "layergrid.h"
#include "layergrid_impl.h"
#include "celltree_impl.h"

//#define INTERPOL_DEBUG

namespace vistle {

LayerGrid::LayerGrid(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const Meta &meta)
: LayerGrid::Base(LayerGrid::Data::create(numVert_x, numVert_y, numVert_z, meta))
{
    refreshImpl();
}

void LayerGrid::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);

    for (int c = 0; c < 3; ++c) {
        m_numDivisions[c] = d ? d->numDivisions[c] : 1;

        m_ghostLayers[c][0] = d ? d->ghostLayers[c][0] : 0;
        m_ghostLayers[c][1] = d ? d->ghostLayers[c][1] : 0;
    }

    for (int c = 0; c < 2; ++c) {
        m_min[c] = d ? d->min[c] : 0.f;
        m_max[c] = d ? d->max[c] : 0.f;
        if (m_numDivisions[c] > 1)
            m_dist[c] = (m_max[c] - m_min[c]) / (m_numDivisions[c] - 1);
        else
            m_dist[c] = 0;
    }
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool LayerGrid::checkImpl() const
{
    V_CHECK(getSize() == getNumDivisions(0) * getNumDivisions(1) * getNumDivisions(2));

    for (int c = 0; c < 3; c++) {
        V_CHECK(d()->ghostLayers[c][0] + d()->ghostLayers[c][1] < getNumDivisions(c));
    }

    for (int c = 0; c < 2; c++) {
        V_CHECK(d()->min[c] <= d()->max[c]);
    }

    return true;
}

void LayerGrid::updateInternals()
{
    Base::applyDimensionHint(shared_from_this());
    Base::updateInternals();
}

// IS EMPTY
//-------------------------------------------------------------------------
bool LayerGrid::isEmpty()
{
    return Base::isEmpty();
}

bool LayerGrid::isEmpty() const
{
    return Base::isEmpty();
}

std::set<Object::const_ptr> LayerGrid::referencedObjects() const
{
    auto objs = Base::referencedObjects();

    auto norm = normals();
    if (norm && objs.emplace(norm).second) {
        auto no = norm->referencedObjects();
        std::copy(no.begin(), no.end(), std::inserter(objs, objs.begin()));
    }

    return objs;
}

void LayerGrid::print(std::ostream &os) const
{
    Base::print(os);
    os << " " << getSize() << "=" << getNumDivisions(0) << "x" << getNumDivisions(1) << "x" << getNumDivisions(2);
}

//-------------------------------------------------------------------------
Index LayerGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos)
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return d()->ghostLayers[dim][layerPosition];
}

// GET FUNCTION - GHOST CELL LAYER CONST
//-------------------------------------------------------------------------
Index LayerGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return m_ghostLayers[dim][layerPosition];
}

void LayerGrid::setGlobalIndexOffset(int c, Index offset)
{
    d()->indexOffset[c] = offset;
}

// SET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
void LayerGrid::setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value)
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    d()->ghostLayers[dim][layerPosition] = value;
    m_ghostLayers[dim][layerPosition] = value;

    return;
}

// HAS CELL TREE CHECK
//-------------------------------------------------------------------------
bool LayerGrid::hasCelltree() const
{
    if (m_celltree)
        return true;

    return hasAttachment("celltree");
}

// GET FUNCTION - CELL TREE
//-------------------------------------------------------------------------
LayerGrid::Celltree::const_ptr LayerGrid::getCelltree() const
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
bool LayerGrid::validateCelltree() const
{
    if (!hasCelltree())
        return false;

    CellBoundsFunctor<Scalar, Index> boundFunc(this);
    auto ct = getCelltree();
    if (!ct->validateTree(boundFunc)) {
        std::cerr << "LayerGrid: Celltree validation failed with " << getNumElements()
                  << " elements total, bounds: " << getBounds().first << "-" << getBounds().second << std::endl;
        return false;
    }
    return true;
}

// CREATE CELL TREE
//-------------------------------------------------------------------------
void LayerGrid::createCelltree(Index dims[3]) const
{
    if (hasCelltree())
        return;

    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 vmin, vmax;
    vmin.fill(-smax);
    vmax.fill(smax);

    const Index nelem = getNumElements();
    std::vector<Celltree::AABB> bounds(nelem);

    const Scalar *z = &this->z()[0];
    Vector3 gmin = vmax, gmax = vmin;
    for (Index el = 0; el < nelem; ++el) {
        Scalar min[3]{smax, smax, smax};
        Scalar max[3]{-smax, -smax, -smax};
        const auto corners = cellVertices(el, dims);
        for (const auto v: corners) {
            auto n = vertexCoordinates(v, m_numDivisions);
            for (int d = 0; d < 3; ++d) {
                if (d == 2) {
                    min[d] = std::min(min[d], z[v]);
                    max[d] = std::min(max[d], z[v]);
                } else {
                    auto xx = m_min[d] + n[d] * m_dist[d];
                    min[d] = std::min(min[d], xx);
                    max[d] = std::min(max[d], xx);
                }
            }
        }
        auto &b = bounds[el];
        for (int d = 0; d < 3; ++d) {
            gmin[d] = std::min(gmin[d], min[d]);
            gmax[d] = std::max(gmax[d], max[d]);
            b.mmin[d] = min[d];
            b.mmax[d] = max[d];
        }
    }

    typename Celltree::ptr ct(new Celltree(nelem));
    ct->init(bounds.data(), gmin, gmax);
    addAttachment("celltree", ct);
#ifndef NDEBUG
    if (!validateCelltree()) {
        std::cerr << "ERROR: Celltree validation failed." << std::endl;
    }
#endif
}

Index LayerGrid::getNumVertices()
{
    return getSize();
    //return getNumDivisions(0) * getNumDivisions(1) * getNumDivisions(2);
}

Index LayerGrid::getNumVertices() const
{
    return getSize();
}

Vector3 LayerGrid::getVertex(Index v) const
{
    auto n = vertexCoordinates(v, m_numDivisions);
    return Vector3(m_min[0] + n[0] * m_dist[0], m_min[1] + n[1] * m_dist[1], x()[v]);
}

// GET FUNCTION - BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector3, Vector3> LayerGrid::getBounds() const
{
    if (hasCelltree()) {
        const auto ct = getCelltree();
        return std::make_pair(Vector3(ct->min()), Vector3(ct->max()));
    }

    auto mm = Base::getMinMax();
    return std::make_pair(Vector3(m_min[0], m_min[1], mm.first[0]), Vector3(m_max[0], m_max[1], mm.second[0]));
}

Normals::const_ptr LayerGrid::normals() const
{
    return Normals::as(d()->normals.getObject());
}

void LayerGrid::setNormals(Normals::const_ptr normals)
{
    assert(!normals || normals->check());
    d()->normals = normals;
}

// CELL BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector3, Vector3> LayerGrid::cellBounds(Index elem) const
{
    const Scalar smax = std::numeric_limits<Scalar>::max();
    const Scalar *z = &this->z()[0];
    auto cl = cellVertices(elem, m_numDivisions);
    auto n = cellCoordinates(elem, m_numDivisions);
    Vector3 min(m_min[0] + n[0] * m_dist[0], m_min[1] + n[1] * m_dist[1], smax);
    Vector3 max(min[0] + m_dist[0], min[1] + m_dist[1], -smax);
    for (Index v: cl) {
        min[2] = std::min(min[2], z[v]);
        max[2] = std::max(max[2], z[v]);
    }
    return std::make_pair(min, max);
}

std::vector<Vector3> LayerGrid::cellCorners(Index elem) const
{
    const Scalar *z = &this->z()[0];
    auto n = cellCoordinates(elem, m_numDivisions);
    auto cl = cellVertices(elem, m_numDivisions);
    std::vector<Vector3> corners(cl.size());
    for (unsigned i = 0; i < cl.size(); ++i) {
        corners[i][0] = m_min[0] + (n[0] + i % 2) * m_dist[0];
        corners[i][1] = m_min[1] + (n[1] + ((i + 1) / 2) % 2) * m_dist[1];
        corners[i][2] = z[cl[i]];
    }

    return corners;
}

// FIND CELL
//-------------------------------------------------------------------------
Index LayerGrid::findCell(const Vector3 &point, Index hint, int flags) const
{
    const bool acceptGhost = flags & AcceptGhost;
    const bool useCelltree = (flags & ForceCelltree) || (hasCelltree() && !(flags & NoCelltree));

    if (hint != InvalidIndex && inside(hint, point)) {
        return hint;
    }

    if (useCelltree) {
        vistle::PointVisitationFunctor<Scalar, Index> nodeFunc(point);
        vistle::PointInclusionFunctor<LayerGrid, Scalar, Index> elemFunc(this, point, acceptGhost);
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
bool LayerGrid::inside(Index elem, const Vector3 &point) const
{
    if (elem == InvalidIndex)
        return false;

    for (int c = 0; c < 2; ++c) {
        if (point[c] < m_min[c])
            return false;
        if (point[c] > m_max[c])
            return false;
    }

    std::array<Index, 3> n = cellCoordinates(elem, m_numDivisions);
    for (int c = 0; c < 2; ++c) {
        Scalar x = m_min[c] + n[c] * m_dist[c];
        if (point[c] < x)
            return false;
        if (point[c] > x + m_dist[c])
            return false;
    }

    auto corners = cellCorners(elem);
    if (corners.size() < 8)
        return false;

    const int faces[2][4] = {{3, 2, 1, 0}, {4, 5, 6, 7}};
    // first two faces are top and bottom
    for (int f = 0; f < 2; ++f) {
        Vector3 v0 = corners[faces[f][0]];
        Vector3 edge1 = corners[faces[f][1]];
        edge1 -= v0;
        Vector3 n(0, 0, 0);
        for (unsigned i = 2; i < 4; ++i) {
            Vector3 edge = corners[faces[f][i]];
            edge -= v0;
            n += cross(edge1, edge);
        }

        //std::cerr << "normal: " << n.transpose() << ", v0: " << v0.transpose() << ", rel: " << (point-v0).transpose() << ", dot: " << n.dot(point-v0) << std::endl;

        if (n.dot(point - v0) > 0)
            return false;
    }
    return true;
}

Scalar LayerGrid::exitDistance(Index elem, const Vector3 &point, const Vector3 &dir) const
{
    refresh();

    Scalar exitDist = -1;
    const Vector3 raydir(dir.normalized());

    auto verts = cellCorners(elem);
    if (verts.size() < 8)
        return exitDist;

    const UnstructuredGrid::Type type = UnstructuredGrid::HEXAHEDRON;
    const auto numFaces = UnstructuredGrid::NumFaces[type];
    const auto &faces = UnstructuredGrid::FaceVertices[type];
    const auto &sizes = UnstructuredGrid::FaceSizes[type];
    Vector3 corners[4];
    for (int f = 0; f < numFaces; ++f) {
        auto nc = faceNormalAndCenter(type, f, verts.data());
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
            const Index v = faces[f][i];
            corners[i] = verts[v];
        }
        const auto isect = point + t * raydir;
        if (insidePolygon(isect, corners, nCorners, normal)) {
            if (exitDist < 0 || t < exitDist)
                exitDist = t;
        }
    }
    return exitDist;
}

void LayerGrid::copyAttributes(Object::const_ptr src, bool replace)
{
    Base::copyAttributes(src, replace);
    if (auto s = StructuredGridBase::as(src)) {
        for (int c = 0; c < 3; ++c)
            setGlobalIndexOffset(c, s->getGlobalIndexOffset(c));
    }
}

GridInterface::Interpolator LayerGrid::getInterpolator(Index elem, const Vector3 &point, DataBase::Mapping mapping,
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
    auto corners = cellCorners(elem);

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
                const Vector3 vert = corners[i];
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

void LayerGrid::Data::initData()
{
    for (int i = 0; i < 3; ++i) {
        indexOffset[i] = 0;
        numDivisions[i] = 0;
        ghostLayers[i][0] = ghostLayers[i][1] = 0;
    }
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
LayerGrid::Data::Data(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z, const std::string &name,
                      const Meta &meta)
: LayerGrid::Base::Data(numVert_x * numVert_y * numVert_z, Object::LAYERGRID, name, meta)
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
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
LayerGrid::Data::Data(const LayerGrid::Data &o, const std::string &n): LayerGrid::Base::Data(o, n)
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
LayerGrid::Data::~Data()
{}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
LayerGrid::Data *LayerGrid::Data::create(const size_t numVert_x, const size_t numVert_y, const size_t numVert_z,
                                         const Meta &meta)
{
    // construct shm data
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numVert_x, numVert_y, numVert_z, name, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(LayerGrid, Object::LAYERGRID)
V_OBJECT_CTOR(LayerGrid)
V_OBJECT_IMPL(LayerGrid)

} // namespace vistle
