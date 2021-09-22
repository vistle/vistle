//-------------------------------------------------------------------------
// UNIFORM GRID CLASS H
// *
// * Uniform Grid Container Object
//-------------------------------------------------------------------------

#include "uniformgrid.h"
#include "uniformgrid_impl.h"
#include "archives.h"
#include <cassert>

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::UniformGrid(Index xdim, Index ydim, Index zdim, const Meta &meta)
: UniformGrid::Base(UniformGrid::Data::create(xdim, ydim, zdim, meta))
{
    refreshImpl();
}

// REFRESH IMPL
//-------------------------------------------------------------------------
void UniformGrid::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);

    for (int c = 0; c < 3; ++c) {
        m_numDivisions[c] = d ? d->numDivisions[c] : 1;
        m_min[c] = d ? d->min[c] : 0.f;
        m_max[c] = d ? d->max[c] : 0.f;
        if (m_numDivisions[c] > 1)
            m_dist[c] = (m_max[c] - m_min[c]) / (m_numDivisions[c] - 1);
        else
            m_dist[c] = 0;

        m_ghostLayers[c][0] = d ? d->ghostLayers[c][0] : 0;
        m_ghostLayers[c][1] = d ? d->ghostLayers[c][1] : 0;
    }
    m_size = m_numDivisions[0] * m_numDivisions[1] * m_numDivisions[2];
}

// CHECK IMPL
//-------------------------------------------------------------------------
bool UniformGrid::checkImpl() const
{
    for (int c = 0; c < 3; c++) {
        V_CHECK(d()->ghostLayers[c][0] + d()->ghostLayers[c][1] < getNumDivisions(c));
        V_CHECK(d()->min[c] <= d()->max[c]);
    }

    return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool UniformGrid::isEmpty()
{
    return Base::isEmpty();
}

bool UniformGrid::isEmpty() const
{
    return Base::isEmpty();
}

// GET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
Index UniformGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos)
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return d()->ghostLayers[dim][layerPosition];
}

// GET FUNCTION - GHOST CELL LAYER CONST
//-------------------------------------------------------------------------
Index UniformGrid::getNumGhostLayers(unsigned dim, GhostLayerPosition pos) const
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    return m_ghostLayers[dim][layerPosition];
}

void UniformGrid::setGlobalIndexOffset(int c, Index offset)
{
    d()->indexOffset[c] = offset;
}

// SET FUNCTION - GHOST CELL LAYER
//-------------------------------------------------------------------------
void UniformGrid::setNumGhostLayers(unsigned dim, GhostLayerPosition pos, unsigned value)
{
    unsigned layerPosition = (pos == Bottom) ? 0 : 1;

    d()->ghostLayers[dim][layerPosition] = value;
    m_ghostLayers[dim][layerPosition] = value;

    return;
}

Index UniformGrid::getNumVertices()
{
    return getNumDivisions(0) * getNumDivisions(1) * getNumDivisions(2);
}

Index UniformGrid::getNumVertices() const
{
    return m_size;
}

// GET BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector, Vector> UniformGrid::getBounds() const
{
    return std::make_pair(Vector(m_min[0], m_min[1], m_min[2]), Vector(m_max[0], m_max[1], m_max[2]));
}

Normals::const_ptr UniformGrid::normals() const
{
    return Normals::as(d()->normals.getObject());
}

void UniformGrid::setNormals(Normals::const_ptr normals)
{
    d()->normals = normals;
}

// CELL BOUNDS
//-------------------------------------------------------------------------
std::pair<Vector, Vector> UniformGrid::cellBounds(Index elem) const
{
    auto n = cellCoordinates(elem, m_numDivisions);
    Vector min(m_min[0] + n[0] * m_dist[0], m_min[1] + n[1] * m_dist[1], m_min[2] + n[2] * m_dist[2]);
    Vector max = min + Vector(m_dist[0], m_dist[1], m_dist[2]);
    return std::make_pair(min, max);
}

// FIND CELL
//-------------------------------------------------------------------------
Index UniformGrid::findCell(const Vector &point, Index hint, int flags) const
{
    const bool acceptGhost = flags & AcceptGhost;

    for (int c = 0; c < 3; ++c) {
        if (point[c] < m_min[c])
            return InvalidIndex;
        if (point[c] > m_max[c])
            return InvalidIndex;
    }

    Index n[3];
    for (int c = 0; c < 3; ++c) {
        n[c] = (point[c] - m_min[c]) / m_dist[c];
        if (n[c] >= m_numDivisions[c] - 1)
            n[c] = m_numDivisions[c] - 2;
    }

    Index el = cellIndex(n, m_numDivisions);
    assert(inside(el, point));
    if (acceptGhost || !isGhostCell(el))
        return el;
    return InvalidIndex;
}

// INSIDE CHECK
//-------------------------------------------------------------------------
bool UniformGrid::inside(Index elem, const Vector &point) const
{
    if (elem == InvalidIndex)
        return false;

    for (int c = 0; c < 3; ++c) {
        if (point[c] < m_min[c])
            return false;
        if (point[c] > m_max[c])
            return false;
    }

    std::array<Index, 3> n = cellCoordinates(elem, m_numDivisions);
    for (int c = 0; c < 3; ++c) {
        Scalar x = m_min[c] + n[c] * m_dist[c];
        if (point[c] < x)
            return false;
        if (point[c] > x + m_dist[c])
            return false;
    }

    return true;
}

Scalar UniformGrid::exitDistance(Index elem, const Vector &point, const Vector &dir) const
{
    if (elem == InvalidIndex)
        return false;

    Vector raydir(dir.normalized());

    std::array<Index, 3> n = cellCoordinates(elem, m_numDivisions);
    assert(n[0] < m_numDivisions[0] - 1);
    assert(n[1] < m_numDivisions[1] - 1);
    assert(n[2] < m_numDivisions[2] - 1);
    Scalar exitDist = -1;
    Scalar t0[3], t1[3];
    for (int c = 0; c < 3; ++c) {
        Scalar x0 = m_min[c] + n[c] * m_dist[c], x1 = x0 + m_dist[c];
        t0[c] = (x0 - point[c]) / raydir[c];
        t1[c] = (x1 - point[c]) / raydir[c];
    }

    for (int c = 0; c < 3; ++c) {
        if (t0[c] > 0) {
            if (exitDist < 0 || exitDist > t0[c])
                exitDist = t0[c];
        }
        if (t1[c] > 0) {
            if (exitDist < 0 || exitDist > t1[c])
                exitDist = t1[c];
        }
    }

    return exitDist;
}

Vector UniformGrid::getVertex(Index v) const
{
    auto n = vertexCoordinates(v, m_numDivisions);
    return Vector(m_min[0] + n[0] * m_dist[0], m_min[1] + n[1] * m_dist[1], m_min[2] + n[2] * m_dist[2]);
}

void UniformGrid::copyAttributes(Object::const_ptr src, bool replace)
{
    Base::copyAttributes(src, replace);
    if (auto s = StructuredGridBase::as(src)) {
        for (int c = 0; c < 3; ++c)
            setGlobalIndexOffset(c, s->getGlobalIndexOffset(c));
    }
}

// GET INTERPOLATOR
//-------------------------------------------------------------------------
GridInterface::Interpolator UniformGrid::getInterpolator(Index elem, const Vector &point, DataBase::Mapping mapping,
                                                         GridInterface::InterpolationMode mode) const
{
    assert(inside(elem, point));

#ifdef INTERPOL_DEBUG
    if (!inside(elem, point)) {
        return Interpolator();
    }
#endif

    if (mapping == DataBase::Element) {
        std::vector<Scalar> weights(1, 1.);
        std::vector<Index> indices(1, elem);

        return Interpolator(weights, indices);
    }

    std::array<Index, 3> n = cellCoordinates(elem, m_numDivisions);
    auto cl = cellVertices(elem, m_numDivisions);

    Vector corner(m_min[0] + n[0] * m_dist[0], m_min[1] + n[1] * m_dist[1], m_min[2] + n[2] * m_dist[2]);
    const Vector diff = point - corner;

    const Index nvert = cl.size();
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
        Vector ss = diff;
        for (int c = 0; c < 3; ++c) {
            ss[c] /= m_dist[c];
            assert(ss[c] >= 0.0);
            assert(ss[c] <= 1.0);
        }
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
            Vector ss = diff;
            int nearest = 0;
            for (int c = 0; c < 3; ++c) {
                nearest <<= 1;
                ss[c] /= m_dist[c];
                if (ss[c] < 0.5)
                    nearest |= 1;
            }
            indices[0] = cl[nearest];
        }
    }

    return Interpolator(weights, indices);
}

void UniformGrid::Data::initData()
{
    for (int i = 0; i < 3; ++i) {
        indexOffset[i] = 0;
        numDivisions[i] = 0;
        ghostLayers[i][0] = ghostLayers[i][1] = 0;
        min[i] = 1;
        max[i] = -1;
    }
}

// DATA OBJECT - CONSTRUCTOR FROM NAME & META
//-------------------------------------------------------------------------
UniformGrid::Data::Data(const std::string &name, Index xdim, Index ydim, Index zdim, const Meta &meta)
: UniformGrid::Base::Data(Object::UNIFORMGRID, name, meta)
{
    initData();

    numDivisions[0] = xdim;
    numDivisions[1] = ydim;
    numDivisions[2] = zdim;
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
UniformGrid::Data::Data(const UniformGrid::Data &o, const std::string &n): UniformGrid::Base::Data(o, n)
{
    initData();

    normals = o.normals;
    for (int i = 0; i < 3; ++i) {
        min[i] = o.min[i];
        max[i] = o.max[i];
        numDivisions[i] = o.numDivisions[i];
    }
}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
UniformGrid::Data::~Data()
{}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
UniformGrid::Data *UniformGrid::Data::create(Index xdim, Index ydim, Index zdim, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(name, xdim, ydim, zdim, meta);
    publish(p);

    return p;
}

// MACROS
//-------------------------------------------------------------------------
V_OBJECT_TYPE(UniformGrid, Object::UNIFORMGRID)
V_OBJECT_CTOR(UniformGrid)
V_OBJECT_IMPL(UniformGrid)

} // namespace vistle
