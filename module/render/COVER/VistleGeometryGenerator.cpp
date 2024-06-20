#include "VistleGeometryGenerator.h"

#include <cmath>
#include <algorithm>
#include <type_traits>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/Texture1D>
#include <osg/TexEnv>
#include <osg/Point>
#include <osg/LineWidth>
#include "HeightMap.h"

#include <vistle/util/math.h>
#include <vistle/core/polygons.h>
#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/placeholder.h>
#include <vistle/core/normals.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/celltypes.h>

#ifdef COVER_PLUGIN
#include <cover/RenderObject.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRShader.h>
#include <cover/coVRPluginSupport.h>
#include <PluginUtil/Tipsify.h>

//#define BUILD_KDTREES
#endif

using namespace vistle;

namespace {
const int NumPrimitives = 100000;
const bool IndexGeo = true;

#ifdef COVER_PLUGIN
const Index TileSize = 256;
const int RadiusAttrib = 11; // nvidia: gl_MultiTexCoord3
#endif
const int TfTexUnit = 0;
const int DataAttrib = 10; // nvidia: gl_MultiTexCoord2
} // namespace

std::mutex VistleGeometryGenerator::s_coverMutex;

namespace {
#ifdef BUILD_KDTREES
std::mutex kdTreeMutex;
std::vector<osg::ref_ptr<osg::KdTreeBuilder>> kdTreeBuilders;
#endif

std::map<std::string, std::string> get_shader_parameters()
{
    std::map<std::string, std::string> parammap;
    parammap["dataAttrib"] = std::to_string(DataAttrib);
#ifdef COVER_PLUGIN
    parammap["texUnit1"] = std::to_string(TfTexUnit);
    parammap["radiusAttrib"] = std::to_string(RadiusAttrib);
#endif
    return parammap;
}

} // namespace

template<class Geo>
struct PrimitiveAdapter {
    PrimitiveAdapter(typename Geo::const_ptr geo)
    : m_geo(geo)
    , m_numPrim(geo->getNumElements())
    , m_x(&geo->x()[0])
    , m_y(&geo->y()[0])
    , m_z(&geo->z()[0])
    , m_cl(m_geo->getNumCorners() > 0 ? &m_geo->cl()[0] : nullptr)
    {}

    Index getNumCoords() const { return m_geo->getNumCoords(); }
    Index getNumVertices() const { return m_geo->getNumCorners(); }
    Index getNumTriangles() const;

    Index getNumPrimitives() const { return m_numPrim; }

    Index getPrimitiveBegin(Index prim) const;

    Vector3 getCoord(Index prim) const
    {
        Index c = this->getPrimitiveBegin(prim);
        if (m_cl)
            c = m_cl[c];

        return Vector3(m_x[c], m_y[c], m_z[c]);
    }

    typename Geo::const_ptr m_geo;
    Index m_numPrim = 0;
    const Scalar *m_x = nullptr, *m_y = nullptr, *m_z = nullptr;
    const Index *m_el = nullptr;
    const Index *m_cl = nullptr;
};

template<>
Index PrimitiveAdapter<Triangles>::getPrimitiveBegin(Index prim) const
{
    return 3 * prim;
}

template<>
Index PrimitiveAdapter<Triangles>::getNumTriangles() const
{
    return m_numPrim;
}

template<>
Index PrimitiveAdapter<Quads>::getPrimitiveBegin(Index prim) const
{
    return 4 * prim;
}

template<>
Index PrimitiveAdapter<Quads>::getNumTriangles() const
{
    return m_numPrim * 2;
}

template<>
PrimitiveAdapter<Indexed>::PrimitiveAdapter(Indexed::const_ptr geo)
: m_geo(geo)
, m_numPrim(geo->getNumElements())
, m_x(&geo->x()[0])
, m_y(&geo->y()[0])
, m_z(&geo->z()[0])
, m_el(&geo->el()[0])
, m_cl(m_geo->getNumCorners() > 0 ? &m_geo->cl()[0] : nullptr)
{}

template<>
Index PrimitiveAdapter<Indexed>::getNumTriangles() const
{
    return getNumVertices() - 2 * getNumPrimitives();
}

template<>
Index PrimitiveAdapter<Indexed>::getPrimitiveBegin(Index prim) const
{
    return m_el[prim];
}

struct PrimitiveBin {
    Index ntri = InvalidIndex; // number of triangles
    std::vector<Index> prim; // primitive indices
    std::vector<Index> vertexMap; // map from old to new vertex indices
    std::vector<Index> ncl; // new connectivity list

    void clear()
    {
        std::vector<Index>().swap(vertexMap);
        std::vector<Index>().swap(ncl);
    }
};

template<class Geo>
std::vector<PrimitiveBin> binPrimitivesRec(int level, const PrimitiveAdapter<Geo> &adp, const Vector3 &bmin,
                                           const Vector3 &bmax, const PrimitiveBin &bin, size_t numPrimitives)
{
    if (bin.prim.size() < numPrimitives || level > 8) {
        std::vector<PrimitiveBin> result;
        if (!bin.prim.empty())
            result.push_back(bin);
        return result;
    }

    auto bsize = bmax - bmin;
    int splitDim = 0;
    if (bsize[1] > bsize[splitDim])
        splitDim = 1;
    if (bsize[2] > bsize[splitDim])
        splitDim = 2;

    auto splitAt = 0.5 * bmin[splitDim] + 0.5 * bmax[splitDim];

    PrimitiveBin binMin, binMax;
    binMin.prim.reserve(bin.prim.size());
    binMax.prim.reserve(bin.prim.size());
    auto splitMin = bmax[splitDim], splitMax = bmin[splitDim];
    for (auto v: bin.prim) {
        auto c = adp.getCoord(v)[splitDim];
        if (c < splitAt) {
            binMin.prim.push_back(v);
            if (c > splitMax)
                splitMax = c;
        } else {
            binMax.prim.push_back(v);
            if (c < splitMin)
                splitMin = c;
        }
    }

    //std::cerr << "level=" << level << ", splitDim=" << splitDim << splitAt << ", #bmin=" << binMin.prim.size() << ", #bmax=" << binMax.prim.size() << std::endl;

    auto smax = bmax;
    smax[splitDim] = splitMax;
    auto rMin = binPrimitivesRec(level + 1, adp, bmin, smax, binMin, numPrimitives);

    auto smin = bmin;
    smin[splitDim] = splitMin;
    auto rMax = binPrimitivesRec(level + 1, adp, smin, bmax, binMax, numPrimitives);

    std::copy(rMin.begin(), rMin.end(), std::back_inserter(rMax));

    return rMax;
}

template<class Geo>
std::vector<PrimitiveBin> binPrimitives(typename Geo::const_ptr geo, size_t numPrimitives)
{
    PrimitiveAdapter<Geo> adp(geo);
    auto bounds = geo->getBounds();

    PrimitiveBin bin;
    bin.prim.reserve(adp.getNumPrimitives());
    for (Index i = 0; i < adp.getNumPrimitives(); ++i)
        bin.prim.emplace_back(i);
    return binPrimitivesRec(0, adp, bounds.first, bounds.second, bin, numPrimitives);
}

VistleGeometryGenerator::VistleGeometryGenerator(std::shared_ptr<vistle::RenderObject> ro,
                                                 vistle::Object::const_ptr geo, vistle::Object::const_ptr normal,
                                                 vistle::Object::const_ptr mapped)
: m_ro(ro), m_geo(geo), m_normal(normal), m_mapped(mapped)
{
    if (m_mapped) {
        m_species = m_mapped->getAttribute("_species");
    }
}

const std::string &VistleGeometryGenerator::species() const
{
    return m_species;
}

void VistleGeometryGenerator::setColorMaps(const OsgColorMapMap *colormaps)
{
    m_colormaps = colormaps;
}

void VistleGeometryGenerator::setGeometryCache(vistle::ResultCache<GeometryCache> &cache)
{
    m_cache = &cache;
}

bool VistleGeometryGenerator::isSupported(vistle::Object::Type t)
{
    switch (t) {
    case vistle::Object::POINTS:
    case vistle::Object::LINES:
    case vistle::Object::TRIANGLES:
    case vistle::Object::QUADS:
    case vistle::Object::POLYGONS:
    case vistle::Object::LAYERGRID:
        return true;

    default:
        return false;
    }
}

void VistleGeometryGenerator::lock()
{
    s_coverMutex.lock();
}

void VistleGeometryGenerator::unlock()
{
    s_coverMutex.unlock();
}

template<class Geometry, class Array, class Mapped, bool normalize>
struct DataAdapter;

template<class Geometry, class Mapped, bool normalize>
struct DataAdapter<Geometry, osg::Vec3Array, Mapped, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename Mapped::const_ptr mapped)
    : size(mapped->getSize())
    , x(size > 0 ? &mapped->x()[0] : nullptr)
    , y(size > 0 ? &mapped->y()[0] : nullptr)
    , z(size > 0 ? &mapped->z()[0] : nullptr)
    , mapping(mapped->guessMapping(tri))
    {}
    osg::Vec3 getValue(Index idx)
    {
        osg::Vec3 val(x[idx], y[idx], z[idx]);
        if (normalize)
            val.normalize();
        return val;
    }
    vistle::Index size = 0;
    const typename Mapped::Scalar *x, *y, *z;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, bool normalize>
struct DataAdapter<Geometry, osg::FloatArray, typename vistle::Vec<Scalar, 3>::const_ptr, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename vistle::Vec<Scalar, 3>::const_ptr mapped)
    : size(mapped->getSize())
    , x(size > 0 ? &mapped->x()[0] : nullptr)
    , y(size > 0 ? &mapped->y()[0] : nullptr)
    , z(size > 0 ? &mapped->z()[0] : nullptr)
    , mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) { return sqrt(x[idx] * x[idx] + y[idx] * y[idx] + z[idx] * z[idx]); }
    vistle::Index size = 0;
    const Scalar *x = nullptr, *y = nullptr, *z = nullptr;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, bool normalize>
struct DataAdapter<Geometry, osg::FloatArray, typename vistle::Vec<Index>::const_ptr, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename vistle::Vec<Index>::const_ptr mapped)
    : size(mapped->getSize()), x(size > 0 ? &mapped->x()[0] : nullptr), mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) { return x[idx]; }
    vistle::Index size = 0;
    const Index *x = nullptr;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, class Mapped, bool normalize>
struct DataAdapter<Geometry, osg::FloatArray, Mapped, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename Mapped::const_ptr mapped)
    : size(mapped->getSize()), x(size > 0 ? &mapped->x()[0] : nullptr), mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) { return x[idx]; }
    vistle::Index size = 0;
    const typename Mapped::Scalar *x = nullptr;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, bool normalize>
struct DataAdapter<Geometry, osg::Vec3Array, osg::Vec3Array, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, osg::Vec3Array *mapped)
    : size(mapped->size()), mapped(size > 0 ? mapped : nullptr)
    {}
    osg::Vec3 getValue(Index idx)
    {
        osg::Vec3 val = (*mapped)[idx];
        if (normalize)
            val.normalize();
        return val;
    }
    vistle::DataBase::Mapping mapping = vistle::DataBase::Vertex;
    vistle::Index size = 0;
    osg::Vec3Array *mapped = nullptr;
};

template<class Geometry, class MappedPtr, class Array, bool normalize>
Array *applyTriangle(typename Geometry::const_ptr tri, MappedPtr mapped, bool indexGeom, PrimitiveBin &bin)
{
    if (!mapped)
        return nullptr;

    auto arr = new Array;
    arr->setBinding(osg::Array::BIND_PER_VERTEX);
    DataAdapter<Geometry, Array, typename std::remove_reference<decltype(*mapped)>::type, normalize> adap(tri, mapped);
    if (adap.mapping == vistle::DataBase::Vertex) {
        if (adap.size < tri->getNumCoords()) {
            std::cerr << "applyTriangle: not enough data (" << adap.size << ") for " << tri->getNumCoords()
                      << " coordinates" << std::endl;
            return nullptr;
        }
    } else if (adap.mapping == vistle::DataBase::Element) {
        if (adap.size < tri->getNumElements()) {
            std::cerr << "applyTriangle: not enough data (" << adap.size << ") for " << tri->getNumElements()
                      << " elements" << std::endl;
            return nullptr;
        }
    } else {
        std::cerr << "applyTriangle: mapping=" << adap.mapping << " not handled" << std::endl;
        return nullptr;
    }

    PrimitiveAdapter<Geometry> geo(tri);

    const Index *cl = nullptr;
    if (tri->getNumCorners() > 0)
        cl = &tri->cl()[0];
    bool buildConn = bin.ntri == InvalidIndex;
    if (buildConn) {
        bin.ntri = 0;
        bin.vertexMap.resize(tri->getNumCoords(), InvalidIndex);
    }
    Index vcount = 0;
    for (Index prim: bin.prim) {
        Index begin = geo.getPrimitiveBegin(prim), end = geo.getPrimitiveBegin(prim + 1);
        if (end - begin < 3) {
            std::cerr << "applyTriangle: primitive has only " << end - begin << " vertices" << std::endl;
            continue;
        }
        if (adap.mapping == vistle::DataBase::Element) {
            auto val = adap.getValue(prim);
            for (Index i = 0; i < end - begin - 2; ++i) {
                if (buildConn)
                    ++bin.ntri;
                arr->push_back(val);
                arr->push_back(val);
                arr->push_back(val);
            }
        } else if (adap.mapping == vistle::DataBase::Vertex) {
            if (indexGeom) {
                for (Index i = begin; i < end - 2; ++i) {
                    if (buildConn)
                        ++bin.ntri;
                    for (Index k = 0; k < 3; ++k) {
                        Index v = k == 0 ? cl[begin] : cl[i + k];
                        Index vm = bin.vertexMap[v];
                        if (vm == InvalidIndex) {
                            assert(buildConn);
                            vm = vcount;
                            bin.vertexMap[v] = vm;
                        }
                        if (vm == vcount) {
                            ++vcount;

                            arr->push_back(adap.getValue(v));
                        }
                        if (buildConn) {
                            bin.ncl.push_back(vm);
                        }
                    }
                }
            } else if (cl) {
                Index vb = cl[begin];
                for (Index i = begin; i < end - 2; ++i) {
                    if (buildConn)
                        ++bin.ntri;
                    Index v1 = cl[i + 1];
                    Index v2 = cl[i + 2];
                    arr->push_back(adap.getValue(vb));
                    arr->push_back(adap.getValue(v1));
                    arr->push_back(adap.getValue(v2));
                }
            } else {
                for (Index v = begin; v < end - 2; ++v) {
                    if (buildConn)
                        ++bin.ntri;
                    arr->push_back(adap.getValue(begin));
                    arr->push_back(adap.getValue(v + 1));
                    arr->push_back(adap.getValue(v + 2));
                }
            }
        }
    }

    return arr;
}

template<class Geometry>
osg::Vec3Array *computeNormals(typename Geometry::const_ptr geometry, bool indexGeom)
{
    PrimitiveAdapter<Geometry> geo(geometry);

    const Index numCorners = geometry->getNumCorners();
    const Index numCoords = geometry->getNumCoords();
    const Index numPrim = geometry->getNumElements();

    const Index *cl = &geometry->cl()[0];
    const vistle::Scalar *x = &geometry->x()[0];
    const vistle::Scalar *y = &geometry->y()[0];
    const vistle::Scalar *z = &geometry->z()[0];

    osg::Vec3Array *normals = new osg::Vec3Array;
    if (numCorners > 0) {
        normals->resize(indexGeom ? numCoords : numCorners);
        for (Index prim = 0; prim < numPrim; ++prim) {
            const Index begin = geo.getPrimitiveBegin(prim), end = geo.getPrimitiveBegin(prim + 1);
            Index v0 = cl[begin + 0], v1 = cl[begin + 1], v2 = cl[begin + 2];
            osg::Vec3 u(x[v0], y[v0], z[v0]);
            osg::Vec3 v(x[v1], y[v1], z[v1]);
            osg::Vec3 w(x[v2], y[v2], z[v2]);
            osg::Vec3 normal = (w - u) ^ (v - u) * -1;
            normal.normalize();

            if (indexGeom) {
                for (Index c = begin; c < end; ++c) {
                    const Index v = cl[c];
                    (*normals)[v] += normal;
                }
            } else {
                for (Index c = begin; c < end; ++c) {
                    const Index v = cl[c];
                    (*normals)[v] = normal;
                }
            }
        }
        if (indexGeom) {
            for (Index v = 0; v < numCoords; ++v)
                (*normals)[v].normalize();
        }
    } else {
        for (Index prim = 0; prim < numPrim; ++prim) {
            const Index begin = geo.getPrimitiveBegin(prim), end = geo.getPrimitiveBegin(prim + 1);
            const Index c = begin;
            osg::Vec3 u(x[c + 0], y[c + 0], z[c + 0]);
            osg::Vec3 v(x[c + 1], y[c + 1], z[c + 1]);
            osg::Vec3 w(x[c + 2], y[c + 2], z[c + 2]);
            osg::Vec3 normal = (w - u) ^ (v - u) * -1;
            normal.normalize();

            for (Index c = begin; c < end; ++c)
                normals->push_back(normal);
        }
    }

    return normals;
}

osg::PrimitiveSet *buildTrianglesFromTriangles(const PrimitiveBin &bin, bool indexGeom, const Byte *ghost)
{
    Index numTri = bin.prim.size();
    Index numCorners = bin.ncl.size();
    if (indexGeom) {
        const Index *cl = bin.ncl.data();
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        for (Index corner = 0; corner < numCorners; corner++) {
            Index el = corner / 3;
            if (ghost && ghost[el] == cell::GHOST)
                continue;
            corners->push_back(cl[corner]);
        }
#ifdef COVER_PLUGIN
        opencover::tipsify(&(*corners)[0], corners->size());
#endif
        assert(ghost || corners->size() == numTri * 3);
        return corners;
    } else if (ghost) {
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        for (Index corner = 0; corner < numTri * 3; corner++) {
            Index el = corner / 3;
            if (ghost && ghost[el] == cell::GHOST)
                continue;
            corners->push_back(corner);
        }
#ifdef COVER_PLUGIN
        opencover::tipsify(&(*corners)[0], corners->size());
#endif
        return corners;
    } else {
        return new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, bin.ntri * 3);
    }

    return nullptr;
}

osg::PrimitiveSet *buildTrianglesFromQuads(const PrimitiveBin &bin, bool indexGeom, const Byte *ghost)
{
    Index numTri = bin.prim.size() * 2;
    Index numCorners = bin.ncl.size();
    if (indexGeom) {
        const Index *cl = bin.ncl.data();
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        for (Index corner = 0; corner < numCorners; corner++) {
            Index el = corner / 6;
            if (ghost && ghost[el] == cell::GHOST)
                continue;
            corners->push_back(cl[corner]);
        }
#ifdef COVER_PLUGIN
        opencover::tipsify(&(*corners)[0], corners->size());
#endif
        assert(ghost || corners->size() == numTri * 3);
        return corners;
    } else if (ghost) {
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        for (Index corner = 0; corner < numTri * 3; corner++) {
            Index el = corner / 6;
            if (ghost && ghost[el] == cell::GHOST)
                continue;
            corners->push_back(corner);
        }
        return corners;
    } else {
        return new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, bin.ntri * 3);
    }

    return nullptr;
}

osg::PrimitiveSet *buildTriangles(const PrimitiveBin &bin, const Index *el, bool indexGeom, const Byte *ghost)
{
    Index numElements = bin.prim.size();
    Index numCorners = bin.ncl.size();
    Index numTri = numCorners - 2 * numElements;
    if (indexGeom) {
        const Index *cl = bin.ncl.data();
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        Index begin = 0;
        Index idx = 0;
        for (auto elem: bin.prim) {
            if (ghost && ghost[elem] == cell::GHOST)
                continue;
            const Index num = el[elem + 1] - el[elem];
            const Index end = begin + num;
            for (Index i = begin; i < end - 2; ++i) {
                for (int k = 0; k < 3; ++k)
                    corners->push_back(cl[idx++]);
            }
            begin = end;
        }
#ifdef COVER_PLUGIN
        opencover::tipsify(&(*corners)[0], corners->size());
#endif
        return corners;
    } else if (ghost) {
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        Index begin = 0;
        Index idx = 0;
        for (auto elem: bin.prim) {
            const Index num = el[elem + 1] - el[elem];
            const Index end = begin + num;
            if (ghost && ghost[elem] == cell::GHOST) {
                idx += 3 * (end - begin - 2);
                continue;
            } else {
                for (Index i = begin; i < end - 2; ++i) {
                    for (int k = 0; k < 3; ++k)
                        corners->push_back(idx++);
                }
            }
            begin = end;
        }
#ifdef COVER_PLUGIN
        opencover::tipsify(&(*corners)[0], corners->size());
#endif
        return corners;
    } else {
        return new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, bin.ntri * 3);
    }

    return nullptr;
}

template<class MappedObject>
float getValue(typename MappedObject::const_ptr data, Index idx)
{
    return data->x()[idx];
}

template<>
float getValue<vistle::Texture1D>(typename vistle::Texture1D::const_ptr data, Index idx)
{
    return data->coords()[idx];
}

template<>
float getValue<vistle::Vec<Scalar, 3>>(typename vistle::Vec<Scalar, 3>::const_ptr data, Index idx)
{
    auto x = data->x()[idx];
    auto y = data->y()[idx];
    auto z = data->z()[idx];
    return sqrt(x * x + y * y + z * z);
}

template<class MappedObject>
osg::FloatArray *buildArray(typename MappedObject::const_ptr data, Coords::const_ptr coords, std::stringstream &debug,
                            bool indexGeom)
{
    if (!data)
        return nullptr;

    vistle::DataBase::Mapping mapping = data->guessMapping();
    if (mapping == vistle::DataBase::Unspecified) {
        std::cerr << "VistleGeometryGenerator: Coords: texture size mismatch, expected: " << coords->getSize()
                  << ", have: " << data->getSize() << std::endl;
        debug << "VistleGeometryGenerator: Coords: texture size mismatch, expected: " << coords->getSize()
              << ", have: " << data->getSize() << std::endl;
        return nullptr;
    }

    auto indexed = Indexed::as(coords);
    osg::FloatArray *tc = nullptr;
    if (mapping == vistle::DataBase::Vertex) {
        tc = new osg::FloatArray;
        const Index *cl = nullptr;
        if (indexed && indexed->getNumCorners() > 0)
            cl = &indexed->cl()[0];
        if (indexGeom || !cl) {
            const auto numCoords = coords->getSize();
            const auto ntc = data->getSize();
            if (numCoords == ntc) {
                for (Index index = 0; index < numCoords; ++index) {
                    tc->push_back(getValue<MappedObject>(data, index));
                }
            } else {
                std::cerr << "VistleGeometryGenerator: Texture1D mismatch: #coord=" << numCoords
                          << ", #texcoord=" << ntc << std::endl;
                debug << "VistleGeometryGenerator: Texture1D mismatch: #coord=" << numCoords << ", #texcoord=" << ntc
                      << std::endl;
            }
        } else if (indexed && cl) {
            const auto el = &indexed->el()[0];
            const auto numElements = indexed->getNumElements();
            for (Index index = 0; index < numElements; ++index) {
                const Index num = el[index + 1] - el[index];
                for (Index n = 0; n < num; n++) {
                    Index v = cl[el[index] + n];
                    tc->push_back(getValue<MappedObject>(data, v));
                }
            }
        }
    } else if (mapping == vistle::DataBase::Element) {
        tc = new osg::FloatArray;
        if (indexed) {
            const auto el = &indexed->el()[0];
            const auto numElements = indexed->getNumElements();
            for (Index index = 0; index < numElements; ++index) {
                const Index num = el[index + 1] - el[index];
                for (Index n = 0; n < num; n++) {
                    tc->push_back(getValue<MappedObject>(data, index));
                }
            }
        } else {
            const auto numCoords = coords->getSize();
            for (Index index = 0; index < numCoords; ++index) {
                tc->push_back(getValue<MappedObject>(data, index));
            }
        }
    } else {
        std::cerr << "VistleGeometryGenerator: unknown data mapping " << mapping << " for Texture1D" << std::endl;
        debug << "VistleGeometryGenerator: unknown data mapping " << mapping << " for Texture1D" << std::endl;
    }

    return tc;
}

template<class MappedObject, typename S>
bool fillTexture(std::stringstream &debug, S *tex, Index sx, Index sy, typename MappedObject::const_ptr data,
                 Index dims[3], Index ox, Index oy, Index layer)
{
    const auto &nx = dims[0], &ny = dims[1], &nz = dims[2];

    if (!data)
        return false;

    vistle::DataBase::Mapping mapping = data->guessMapping(data->grid() ? data->grid() : data);
    if (mapping == vistle::DataBase::Unspecified) {
        std::cerr << "VistleGeometryGenerator: fillTexture: cannot determine mapping for " << data->getName()
                  << std::endl;
        debug << "VistleGeometryGenerator: fillTexture: cannot determine mapping for " << data->getName() << std::endl;
        return false;
    }

    if (mapping == vistle::DataBase::Vertex) {
        const Index n = nx * ny * nz;
        if (n != data->getSize()) {
            std::cerr << "VistleGeometryGenerator: fillTexture: expecting " << nx << "x" << ny << "x" << nz << " = "
                      << n << " vertex values, but got " << data->getSize() << std::endl;
            debug << "VistleGeometryGenerator: fillTexture: expecting " << nx << "x" << ny << "x" << nz << " = " << n
                  << " vertex values, but got " << data->getSize() << std::endl;
            return false;
        }

        Index idx = 0;
        for (Index y = 0; y < sy; ++y) {
            for (Index x = 0; x < sx; ++x) {
                Index sidx = vistle::StructuredGridBase::vertexIndex(ox + x, oy + y, layer, dims);
                tex[idx] = getValue<MappedObject>(data, sidx);
                ++idx;
            }
        }

        return true;
    } else if (mapping == vistle::DataBase::Element) {
        const Index n = (nx - 1) * (ny - 1) * (nz - 1);
        if (n != data->getSize()) {
            std::cerr << "VistleGeometryGenerator: fillTexture: expecting " << nx - 1 << "x" << ny - 1 << "x" << nz - 1
                      << " = " << n << " element values, but got " << data->getSize() << std::endl;
            debug << "VistleGeometryGenerator: fillTexture: expecting " << nx - 1 << "x" << ny - 1 << "x" << nz - 1
                  << " = " << n << " element values, but got " << data->getSize() << std::endl;
            return false;
        }

        Index idx = 0;
        for (Index y = 0; y < sy - 1; ++y) {
            for (Index x = 0; x < sx - 1; ++x) {
                Index sidx = vistle::StructuredGridBase::vertexIndex(x + ox, y + oy, layer, dims);
                tex[idx] = getValue<MappedObject>(data, sidx);
                ++idx;
            }
        }

        return true;
    }

    return false;
}

const OsgColorMap *VistleGeometryGenerator::getColorMap(const std::string &species) const
{
#ifdef COVER_PLUGIN
    std::lock_guard<std::mutex> lock(s_coverMutex);
    if (m_colormaps && !m_species.empty()) {
        auto it = m_colormaps->find(m_species);
        if (it != m_colormaps->end()) {
            return &it->second;
        }
    }
#endif
    return nullptr;
}

osg::Geode *VistleGeometryGenerator::operator()(osg::ref_ptr<osg::StateSet> defaultState)
{
    if (m_ro)
        m_ro->updateBounds();

    if (!m_geo)
        return nullptr;

    std::string nodename = m_geo->getName();

    std::stringstream debug;
    debug << nodename << " ";
    debug << "[";
    debug << (m_geo ? "G" : ".");
    debug << (m_normal ? "N" : ".");
    debug << (m_mapped ? "T" : ".");
    debug << "] ";

    int t = m_geo->getTimestep();
    if (t < 0 && m_normal)
        t = m_normal->getTimestep();
    if (t < 0 && m_mapped)
        t = m_mapped->getTimestep();

    int b = m_geo->getBlock();
    if (b < 0 && m_normal)
        b = m_normal->getBlock();
    if (b < 0 && m_mapped)
        b = m_mapped->getBlock();

    debug << "b " << b << ", t " << t << " ";

    bool lighted = true;
    osg::Geode *geode = new osg::Geode();
    geode->setName(nodename);

    std::vector<osg::ref_ptr<osg::Drawable>> draw;
    osg::ref_ptr<osg::StateSet> state;

    bool transparent = false;
    if (m_geo && m_geo->hasAttribute("_transparent")) {
        transparent = m_geo->getAttribute("_transparent") != "false";
    }

    size_t numPrimitives = NumPrimitives;
    if (m_geo && m_geo->hasAttribute("_bin_num_primitives")) {
        auto np = m_geo->getAttribute("_bin_num_primitives");
        numPrimitives = atol(np.c_str());
    }
    vistle::Points::const_ptr points = vistle::Points::as(m_geo);

    osg::Material *mat = nullptr;
    if (m_ro && m_ro->hasSolidColor) {
        const auto &c = m_ro->solidColor;
        mat = new osg::Material;
        mat->setName(nodename + ".material");
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(c[0], c[1], c[2], c[3]));
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(c[0], c[1], c[2], c[3]));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2, 0.2, 0.2, c[3]));
        mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        if (c[3] > 0.f && c[3] < 1.f)
            transparent = true;
    }

    if (defaultState) {
        state = new osg::StateSet(*defaultState);
    } else {
#ifdef COVER_PLUGIN
        if (transparent) {
            state = opencover::VRSceneGraph::instance()->loadTransparentGeostate();
        } else {
            state = opencover::VRSceneGraph::instance()->loadDefaultGeostate();
        }
#endif
    }
    if (!state) {
        state = new osg::StateSet;
        if (transparent) {
            state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            state->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
    }
    state->setName(nodename + ".state");
    if (mat)
        state->setAttribute(mat);

    bool indexGeom = IndexGeo;
    vistle::Normals::const_ptr normals = vistle::Normals::as(m_normal);
    if (normals) {
        auto m = normals->guessMapping(m_geo);
        if (m != vistle::DataBase::Vertex) {
            indexGeom = false;
            debug << "NoIndex: normals ";
        }
    }

    bool dataValid = false;
    bool textureValid = false;
#ifdef COVER_PLUGIN
    const OsgColorMap *colormap = nullptr;
    bool haveSpheres = false;
    bool correctDepth = true;
    if (m_geo && m_geo->hasAttribute("_approximate_depth")) {
        correctDepth = m_geo->getAttribute("_approximate_depth") != "true";
    }
#endif
    vistle::DataBase::const_ptr database = vistle::DataBase::as(m_mapped);
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
    if (database) {
        mapping = database->guessMapping(m_geo);
        if (mapping != vistle::DataBase::Vertex) {
            indexGeom = false;
            debug << "NoIndex: data ";
        }
    }

    vistle::Texture1D::const_ptr tex = vistle::Texture1D::as(m_mapped);
    vistle::Vec<Scalar>::const_ptr sdata = vistle::Vec<Scalar>::as(m_mapped);
    vistle::Vec<Scalar, 3>::const_ptr vdata = vistle::Vec<Scalar, 3>::as(m_mapped);
    vistle::Vec<Index>::const_ptr idata = vistle::Vec<Index>::as(m_mapped);
    vistle::Vec<Byte>::const_ptr bdata = vistle::Vec<Byte>::as(m_mapped);
    if (tex) {
        sdata.reset();
    } else if (sdata || vdata || idata || bdata) {
#ifdef COVER_PLUGIN
        colormap = getColorMap(m_species);
#endif
    } else if (database) {
        debug << "Unsupported mapped data: type=" << Object::toString(database->getType()) << " ("
              << database->getType() << ")";
    }

    GeometryCache cache;
    bool cached = false;
    ResultCache<GeometryCache>::Entry *cacheEntry = nullptr;
    if (m_cache) {
        cacheEntry = m_cache->getOrLock(m_geo->getName(), cache);
        if (!cacheEntry)
            cached = true;
        if (cached)
            debug << "cached ";
    }

    const Byte *ghost = nullptr;
    switch (m_geo->getType()) {
    case vistle::Object::PLACEHOLDER: {
        vistle::PlaceHolder::const_ptr ph = vistle::PlaceHolder::as(m_geo);
        debug << "Placeholder [" << ph->originalName() << "]";
        if (isSupported(ph->originalType())) {
            nodename = ph->originalName();
            geode->setName(nodename);
        }
        break;
    }

    case vistle::Object::POINTS: {
        indexGeom = false;

        assert(points);
        const Index numVertices = points->getNumPoints();

        auto radius = points->radius();

        debug << "Points: [ #v " << numVertices << (radius ? " with radius" : "") << " ]";
        const vistle::Scalar *x = &points->x()[0];
        const vistle::Scalar *y = &points->y()[0];
        const vistle::Scalar *z = &points->z()[0];

        state->setAttribute(new osg::Point(2.0f), osg::StateAttribute::ON);

        auto geom = new osg::Geometry();
        draw.push_back(geom);

        if (numVertices > 0) {
            if (cached) {
                std::unique_lock<GeometryCache> guard(cache);
                geom->setVertexArray(cache.vertices.front());
                geom->addPrimitiveSet(cache.primitives.front());
            } else {
                osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
                for (Index v = 0; v < numVertices; v++)
                    vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

                geom->setVertexArray(vertices.get());
                auto ps = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, numVertices);
                geom->addPrimitiveSet(ps);
                if (m_cache) {
                    cache.vertices.push_back(vertices);
                    cache.primitives.push_back(ps);
                }
            }

#ifdef COVER_PLUGIN
            if (radius) {
                haveSpheres = true;
                const vistle::Scalar *r = &radius->x()[0];

                osg::ref_ptr<osg::FloatArray> rad = new osg::FloatArray();
                rad->reserve(numVertices);
                for (Index v = 0; v < numVertices; v++)
                    rad->push_back(r[v]);
                geom->setVertexAttribArray(RadiusAttrib, rad, osg::Array::BIND_PER_VERTEX);

                geom->setUseDisplayList(false);
                geom->setSupportsDisplayList(false);
                geom->setUseVertexBufferObjects(true);

                if (!colormap) {
                    // required for applying shader
                    colormap = getColorMap("");
                }
            } else
#endif
            {
                lighted = false;
            }
        }
        break;
    }
#ifdef COVER_PLUGIN
    case vistle::Object::LAYERGRID: {
        static_assert(TileSize >= 4,
                      "TileSize needs to be at least for (start and end border and repeated border vertices)");
        vistle::LayerGrid::const_ptr lg = vistle::LayerGrid::as(m_geo);
        const Index numVertices = lg->getNumVertices();

        debug << "LayerGrid: [ #v " << numVertices << " (" << lg->getNumDivisions(0) << " " << lg->getNumDivisions(1)
              << " " << lg->getNumDivisions(2) << ") ]";

        Index dims[3] = {
            lg->getNumDivisions(0),
            lg->getNumDivisions(1),
            lg->getNumDivisions(2),
        };

        HeightMap::DataMode dmode = HeightMap::NoData;
        if (mapping == DataBase::Vertex)
            dmode = HeightMap::VertexData;
        else if (mapping == DataBase::Element)
            dmode = HeightMap::CellData;

        if (dmode == HeightMap::NoData) {
            colormap = getColorMap("");
        } else {
            dataValid = true;
        }

        const auto dx = lg->dist()[0];
        const auto dy = lg->dist()[1];
        for (Index l = 0; l < dims[2]; ++l) {
            std::array<unsigned int, 4> borderWidth = {0, 0, 0, 0};
            borderWidth[2] = 0;
            borderWidth[3] = (TileSize >= dims[1]) ? 0 : 1;
            for (Index y = 0; y < dims[1]; y += TileSize - 1 - 2 * borderWidth[3]) {
                Index sy = std::min(TileSize, dims[1] - y);
                borderWidth[3] = (y + TileSize - borderWidth[2] >= dims[1]) ? 0 : 1;
                //std::cerr << "y=" << y << ", b=" << borderWidth[2] << "," << borderWidth[3] << std::endl;
                //debug << "y=" << y << ", b=" << borderWidth[2] << "," << borderWidth[3] << std::endl;
                borderWidth[0] = 0;
                borderWidth[1] = (TileSize >= dims[0]) ? 0 : 1;
                for (Index x = 0; x < dims[0]; x += TileSize - 1 - 2 * borderWidth[1]) {
                    Index sx = std::min(TileSize, dims[0] - x);
                    borderWidth[1] = (x + TileSize - borderWidth[0] >= dims[0]) ? 0 : 1;

                    auto hf = new HeightMap;
                    hf->setOrigin(osg::Vec3(lg->min()[0] + dx * x, lg->min()[1] + dy * y, 0.));
                    hf->setXInterval(dx);
                    hf->setYInterval(dy);

                    hf->setBorderWidth(borderWidth);
                    hf->allocate(sx, sy, dmode);
                    auto height = hf->getHeights();
                    auto data = hf->getData();

                    fillTexture<LayerGrid>(debug, height, sx, sy, lg, dims, x, y, l);
                    if (sdata) {
                        fillTexture<Vec<Scalar, 1>>(debug, data, sx, sy, sdata, dims, x, y, l);
                    } else if (vdata) {
                        fillTexture<Vec<Scalar, 3>>(debug, data, sx, sy, vdata, dims, x, y, l);
                    } else if (idata) {
                        fillTexture<Vec<Index>>(debug, data, sx, sy, idata, dims, x, y, l);
                    } else if (bdata) {
                        fillTexture<Vec<Byte>>(debug, data, sx, sy, bdata, dims, x, y, l);
                    } else if (tex) {
                        fillTexture<Texture1D>(debug, data, sx, sy, tex, dims, x, y, l);
                    }

                    hf->dirty();
                    hf->build();
                    draw.push_back(hf);

                    borderWidth[0] = borderWidth[1];
                }

                borderWidth[2] = borderWidth[3];
            }
        }
        indexGeom = false;
        break;
    }
#endif

    case vistle::Object::TRIANGLES: {
        vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);

        const Index numCorners = triangles->getNumCorners();
        const Index numVertices = triangles->getNumVertices();
        if (numCorners == 0)
            indexGeom = false;
        if (triangles->ghost().size() > 0)
            ghost = triangles->ghost().data();

        debug << "Triangles: [ #c " << numCorners << ", #v " << numVertices << ", indexed=" << (indexGeom ? "t" : "f")
              << " ]";

        osg::ref_ptr<osg::Vec3Array> gnormals;
        if (!cached && !normals)
            gnormals = computeNormals<vistle::Triangles>(triangles, indexGeom);

        auto bins = binPrimitives<vistle::Triangles>(triangles, numPrimitives);
        debug << " #bins: " << bins.size();

        unsigned nbin = 0;
        for (auto bin: bins) {
            auto geom = new osg::Geometry();
            draw.push_back(geom);

            if (cached) {
                std::unique_lock<GeometryCache> guard(cache);
                geom->setVertexArray(cache.vertices[nbin]);
                geom->addPrimitiveSet(cache.primitives[nbin]);
                geom->setNormalArray(cache.normals[nbin]);
            } else {
                osg::ref_ptr<osg::Vec3Array> vertices =
                    applyTriangle<Triangles, Triangles::const_ptr, osg::Vec3Array, false>(triangles, triangles,
                                                                                          indexGeom, bin);
                geom->setVertexArray(vertices);
                cache.vertices.push_back(vertices);

                auto ps = buildTrianglesFromTriangles(bin, indexGeom, ghost);
                geom->addPrimitiveSet(ps);
                cache.primitives.push_back(ps);

                osg::ref_ptr<osg::Vec3Array> norm = applyTriangle<Triangles, Normals::const_ptr, osg::Vec3Array, true>(
                    triangles, normals, indexGeom, bin);
                if (!norm)
                    norm = applyTriangle<Triangles, osg::Vec3Array *, osg::Vec3Array, false>(triangles, gnormals.get(),
                                                                                             indexGeom, bin);
                geom->setNormalArray(norm.get());
                cache.normals.push_back(norm);
            }
            osg::ref_ptr<osg::FloatArray> fl;
            auto tc = applyTriangle<Triangles, vistle::Texture1D::const_ptr, osg::FloatArray, false>(triangles, tex,
                                                                                                     indexGeom, bin);
            if (tc) {
                geom->setTexCoordArray(TfTexUnit, tc);
                textureValid = true;
            } else if (sdata) {
                fl = applyTriangle<Triangles, vistle::Vec<Scalar>::const_ptr, osg::FloatArray, false>(triangles, sdata,
                                                                                                      indexGeom, bin);
            } else if (vdata) {
                fl = applyTriangle<Triangles, vistle::Vec<Scalar, 3>::const_ptr, osg::FloatArray, false>(
                    triangles, vdata, indexGeom, bin);
            } else if (idata) {
                fl = applyTriangle<Triangles, vistle::Vec<Index>::const_ptr, osg::FloatArray, false>(triangles, idata,
                                                                                                     indexGeom, bin);
            } else if (bdata) {
                fl = applyTriangle<Triangles, vistle::Vec<Byte>::const_ptr, osg::FloatArray, false>(triangles, bdata,
                                                                                                    indexGeom, bin);
            }
            if (fl) {
                geom->setVertexAttribArray(DataAttrib, fl);
                dataValid = true;
            }

            bin.clear();
            ++nbin;
        }

        break;
    }

    case vistle::Object::QUADS: {
        vistle::Quads::const_ptr quads = vistle::Quads::as(m_geo);

        const Index numCorners = quads->getNumCorners();
        const Index numVertices = quads->getNumVertices();
        if (numCorners == 0)
            indexGeom = false;
        if (quads->ghost().size() > 0)
            ghost = quads->ghost().data();

        debug << "Quads: [ #c " << numCorners << ", #v " << numVertices << ", indexed=" << (indexGeom ? "t" : "f")
              << " ]";

        osg::ref_ptr<osg::Vec3Array> gnormals;
        if (!cached && !normals)
            gnormals = computeNormals<vistle::Quads>(quads, indexGeom);

        auto bins = binPrimitives<vistle::Quads>(quads, numPrimitives);
        debug << " #bins: " << bins.size();

        unsigned nbin = 0;
        for (auto bin: bins) {
            auto geom = new osg::Geometry();
            draw.push_back(geom);

            if (cached) {
                std::unique_lock<GeometryCache> guard(cache);
                geom->setVertexArray(cache.vertices[nbin]);
                geom->addPrimitiveSet(cache.primitives[nbin]);
                geom->setNormalArray(cache.normals[nbin]);
            } else {
                osg::ref_ptr<osg::Vec3Array> vertices =
                    applyTriangle<Quads, Quads::const_ptr, osg::Vec3Array, false>(quads, quads, indexGeom, bin);
                geom->setVertexArray(vertices);
                cache.vertices.push_back(vertices);

                auto ps = buildTrianglesFromQuads(bin, indexGeom, ghost);
                geom->addPrimitiveSet(ps);
                cache.primitives.push_back(ps);

                osg::ref_ptr<osg::Vec3Array> norm =
                    applyTriangle<Quads, Normals::const_ptr, osg::Vec3Array, true>(quads, normals, indexGeom, bin);
                if (!norm)
                    norm = applyTriangle<Quads, osg::Vec3Array *, osg::Vec3Array, false>(quads, gnormals.get(),
                                                                                         indexGeom, bin);
                geom->setNormalArray(norm.get());
                cache.normals.push_back(norm);
            }

            osg::ref_ptr<osg::FloatArray> fl;
            auto tc =
                applyTriangle<Quads, vistle::Texture1D::const_ptr, osg::FloatArray, false>(quads, tex, indexGeom, bin);
            if (tc) {
                geom->setTexCoordArray(TfTexUnit, tc);
                textureValid = true;
            } else if (sdata) {
                fl = applyTriangle<Quads, vistle::Vec<Scalar>::const_ptr, osg::FloatArray, false>(quads, sdata,
                                                                                                  indexGeom, bin);
            } else if (vdata) {
                fl = applyTriangle<Quads, vistle::Vec<Scalar, 3>::const_ptr, osg::FloatArray, false>(quads, vdata,
                                                                                                     indexGeom, bin);
            } else if (idata) {
                fl = applyTriangle<Quads, vistle::Vec<Index>::const_ptr, osg::FloatArray, false>(quads, idata,
                                                                                                 indexGeom, bin);
            }
            if (fl) {
                geom->setVertexAttribArray(DataAttrib, fl);
                dataValid = true;
            }

            bin.clear();
            ++nbin;
        }

        break;
    }

    case vistle::Object::POLYGONS: {
        vistle::Polygons::const_ptr polygons = vistle::Polygons::as(m_geo);

        const Index numElements = polygons->getNumElements();
        const Index numCorners = polygons->getNumCorners();
        const Index numVertices = polygons->getNumVertices();
        if (numCorners == 0)
            indexGeom = false;
        if (polygons->ghost().size() > 0)
            ghost = polygons->ghost().data();

        debug << "Polygons: [ #c " << numCorners << ", #e " << numElements << ", #v " << numVertices
              << ", indexed=" << (indexGeom ? "t" : "f") << " ]";

        const Index *el = &polygons->el()[0];

        osg::ref_ptr<osg::Vec3Array> gnormals;
        if (!cached && !normals)
            gnormals = computeNormals<vistle::Indexed>(polygons, indexGeom);

        auto bins = binPrimitives<vistle::Indexed>(polygons, numPrimitives);
        debug << " #bins: " << bins.size();

        unsigned nbin = 0;
        for (auto bin: bins) {
            auto geom = new osg::Geometry();
            draw.push_back(geom);

            if (cached) {
                std::unique_lock<GeometryCache> guard(cache);
                geom->setVertexArray(cache.vertices[nbin]);
                geom->addPrimitiveSet(cache.primitives[nbin]);
                geom->setNormalArray(cache.normals[nbin]);
            } else {
                osg::ref_ptr<osg::Vec3Array> vertices =
                    applyTriangle<Indexed, Polygons::const_ptr, osg::Vec3Array, false>(polygons, polygons, indexGeom,
                                                                                       bin);
                geom->setVertexArray(vertices);
                cache.vertices.push_back(vertices);

                auto ps = buildTriangles(bin, el, indexGeom, ghost);
                geom->addPrimitiveSet(ps);
                cache.primitives.push_back(ps);

                osg::ref_ptr<osg::Vec3Array> norm =
                    applyTriangle<Indexed, Normals::const_ptr, osg::Vec3Array, true>(polygons, normals, indexGeom, bin);
                if (!norm)
                    norm = applyTriangle<Indexed, osg::Vec3Array *, osg::Vec3Array, false>(polygons, gnormals.get(),
                                                                                           indexGeom, bin);
                geom->setNormalArray(norm.get());
                cache.normals.push_back(norm);
            }

            osg::ref_ptr<osg::FloatArray> fl;
            auto tc = applyTriangle<Indexed, vistle::Texture1D::const_ptr, osg::FloatArray, false>(polygons, tex,
                                                                                                   indexGeom, bin);
            if (tc) {
                geom->setTexCoordArray(TfTexUnit, tc);
                textureValid = true;
            } else if (sdata) {
                fl = applyTriangle<Indexed, vistle::Vec<Scalar>::const_ptr, osg::FloatArray, false>(polygons, sdata,
                                                                                                    indexGeom, bin);
            } else if (vdata) {
                fl = applyTriangle<Indexed, vistle::Vec<Scalar, 3>::const_ptr, osg::FloatArray, false>(polygons, vdata,
                                                                                                       indexGeom, bin);
            } else if (idata) {
                fl = applyTriangle<Indexed, vistle::Vec<Index>::const_ptr, osg::FloatArray, false>(polygons, idata,
                                                                                                   indexGeom, bin);
            }
            if (fl) {
                geom->setVertexAttribArray(DataAttrib, fl);
                dataValid = true;
            }

            bin.clear();
            ++nbin;
        }

        break;
    }

    case vistle::Object::LINES: {
        indexGeom = false;

        vistle::Lines::const_ptr lines = vistle::Lines::as(m_geo);
        const Index numElements = lines->getNumElements();
        const Index numCorners = lines->getNumCorners();

        debug << "Lines: [ #c " << numCorners << ", #e " << numElements << " ]";

        auto geom = new osg::Geometry();
        draw.push_back(geom);

        if (cached) {
            std::unique_lock<GeometryCache> guard(cache);
            geom->setVertexArray(cache.vertices.front());
            geom->addPrimitiveSet(cache.primitives.front());
        } else {
            const Index *el = &lines->el()[0];
            const Index *cl = numCorners > 0 ? &lines->cl()[0] : nullptr;
            const vistle::Scalar *x = &lines->x()[0];
            const vistle::Scalar *y = &lines->y()[0];
            const vistle::Scalar *z = &lines->z()[0];

            osg::ref_ptr<osg::DrawArrayLengths> primitives = new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();

            for (Index index = 0; index < numElements; index++) {
                Index start = el[index];
                Index end = el[index + 1];
                Index num = end - start;

                primitives->push_back(num);

                for (Index n = 0; n < num; n++) {
                    Index v = start + n;
                    if (cl)
                        v = cl[v];
                    vertices->push_back(osg::Vec3(x[v], y[v], z[v]));
                }
            }
            geom->setVertexArray(vertices.get());
            cache.vertices.push_back(vertices);

            geom->addPrimitiveSet(primitives.get());
            cache.primitives.push_back(primitives);
        }

        if (m_ro->hasSolidColor) {
            const auto &c = m_ro->solidColor;
            osg::Vec4Array *colArray = new osg::Vec4Array();
            colArray->push_back(osg::Vec4(c[0], c[1], c[2], c[3]));
            colArray->setBinding(osg::Array::BIND_OVERALL);
            geom->setColorArray(colArray);
        }

        lighted = false;
        state->setAttributeAndModes(new osg::LineWidth(2.f), osg::StateAttribute::ON);

        break;
    }

    default:
        assert(isSupported(m_geo->getType()) == false);
        break;
    }

    if (m_cache && !cached) {
        m_cache->storeAndUnlock(cacheEntry, cache);
    }

    // set shader parameters
    std::map<std::string, std::string> parammap;
    std::string shadername = m_geo->getAttribute("shader");
    std::string shaderparams = m_geo->getAttribute("shader_params");
    // format has to be '"key=value" "key=value1 value2"'
    bool escaped = false;
    std::string::size_type keyvaluestart = std::string::npos;
    for (std::string::size_type i = 0; i < shaderparams.length(); ++i) {
        if (!escaped) {
            if (shaderparams[i] == '\\') {
                escaped = true;
                continue;
            }
            if (shaderparams[i] == '"') {
                if (keyvaluestart == std::string::npos) {
                    keyvaluestart = i + 1;
                } else {
                    std::string keyvalue = shaderparams.substr(keyvaluestart, i - keyvaluestart - 1);
                    std::string::size_type eq = keyvalue.find('=');
                    if (eq == std::string::npos) {
                        std::cerr << "ignoring " << keyvalue << ": no '=' sign" << std::endl;
                        debug << "ignoring " << keyvalue << ": no '=' sign" << std::endl;
                    } else {
                        std::string key = keyvalue.substr(0, eq);
                        std::string value = keyvalue.substr(eq + 1);
                        //std::cerr << "found key: " << key << ", value: " << value << std::endl;
                        parammap.insert(std::make_pair(key, value));
                    }
                }
            }
        }
        escaped = false;
    }

    vistle::Coords::const_ptr coords = vistle::Coords::as(m_geo);
    vistle::Indexed::const_ptr indexed = vistle::Indexed::as(m_geo);
    vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);
    vistle::Quads::const_ptr quads = vistle::Quads::as(m_geo);
    vistle::Polygons::const_ptr polygons = vistle::Polygons::as(m_geo);
    vistle::LayerGrid::const_ptr lg = vistle::LayerGrid::as(m_geo);

    state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    state->setMode(GL_LIGHTING, lighted ? osg::StateAttribute::ON : osg::StateAttribute::OFF);

#ifdef COVER_PLUGIN
    if (lg) {
    } else
#endif
        if (coords) {
        osg::Geometry *geom = nullptr;
        if (!draw.empty())
            geom = draw[0]->asGeometry();
        if (tex) {
            osg::ref_ptr<osg::FloatArray> tc;
            if (!triangles && !polygons && !quads) {
                tc = buildArray<vistle::Texture1D>(tex, coords, debug, indexGeom);
                if (tc && !tc->empty() && geom) {
                    geom->setTexCoordArray(TfTexUnit, tc);
                    textureValid = true;
                }
            }
            if (tc || (tex && triangles) || (tex && polygons) || (tex && quads)) {
                osg::ref_ptr<osg::Texture1D> osgTex = new osg::Texture1D;
                osgTex->setName(nodename + ".tex");
                osgTex->setDataVariance(osg::Object::DYNAMIC);
                osgTex->setResizeNonPowerOfTwoHint(false);

                osg::ref_ptr<osg::Image> image = new osg::Image();
                image->setName(nodename + ".img");
                image->setImage(tex->getWidth(), 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                                const_cast<unsigned char *>(&tex->pixels()[0]), osg::Image::NO_DELETE);
                osgTex->setImage(image);

                state->setTextureAttributeAndModes(TfTexUnit, osgTex, osg::StateAttribute::ON);
                osgTex->setFilter(osg::Texture1D::MIN_FILTER, osg::Texture1D::NEAREST);
                osgTex->setFilter(osg::Texture1D::MAG_FILTER, osg::Texture1D::NEAREST);
#if 0
               osg::TexEnv * texEnv = new osg::TexEnv();
               texEnv->setMode(osg::TexEnv::MODULATE);
               state->setTextureAttributeAndModes(TfTexUnit, texEnv);
#endif
            }
        } else if (triangles || polygons || quads) {
            // objects split and mapped data already applied accordingly
        } else if (vistle::Vec<Scalar>::const_ptr data = vistle::Vec<Scalar>::as(m_mapped)) {
            osg::ref_ptr<osg::FloatArray> fl = buildArray<vistle::Vec<Scalar>>(data, coords, debug, indexGeom);
            if (fl && !fl->empty() && geom) {
                //std::cerr << "VistleGeometryGenerator: setting VertexAttribArray for Vec<Scalar> of size " << fl->size() << std::endl;
                geom->setVertexAttribArray(DataAttrib, fl, osg::Array::BIND_PER_VERTEX);
                dataValid = true;
            }
        } else if (vistle::Vec<Scalar, 3>::const_ptr data = vistle::Vec<Scalar, 3>::as(m_mapped)) {
            osg::ref_ptr<osg::FloatArray> fl = buildArray<vistle::Vec<Scalar, 3>>(data, coords, debug, indexGeom);
            if (fl && !fl->empty() && geom) {
                //std::cerr << "VistleGeometryGenerator: setting VertexAttribArray for Vec<Scalar,3> of size " << fl->size() << std::endl;
                geom->setVertexAttribArray(DataAttrib, fl, osg::Array::BIND_PER_VERTEX);
                dataValid = true;
            }
        } else if (vistle::Vec<Index>::const_ptr data = vistle::Vec<Index>::as(m_mapped)) {
            osg::ref_ptr<osg::FloatArray> fl = buildArray<vistle::Vec<Index>>(data, coords, debug, indexGeom);
            if (fl && !fl->empty() && geom) {
                //std::cerr << "VistleGeometryGenerator: setting VertexAttribArray for Vec<Index> of size " << fl->size() << std::endl;
                geom->setVertexAttribArray(DataAttrib, fl, osg::Array::BIND_PER_VERTEX);
                dataValid = true;
            }
        } else if (vistle::Vec<Byte>::const_ptr data = vistle::Vec<Byte>::as(m_mapped)) {
            osg::ref_ptr<osg::FloatArray> fl = buildArray<vistle::Vec<Byte>>(data, coords, debug, indexGeom);
            if (fl && !fl->empty() && geom) {
                //std::cerr << "VistleGeometryGenerator: setting VertexAttribArray for Vec<Byte> of size " << fl->size() << std::endl;
                geom->setVertexAttribArray(DataAttrib, fl, osg::Array::BIND_PER_VERTEX);
                dataValid = true;
            }
        }
    }

#ifdef COVER_PLUGIN
    if (colormap) {
        if (!textureValid && dataValid) {
            state->setTextureAttributeAndModes(TfTexUnit, colormap->texture, osg::StateAttribute::ON);
        }
        s_coverMutex.lock();
        if (haveSpheres) {
            if (correctDepth && colormap->shaderSpheresCorrectDepth)
                colormap->shaderSpheresCorrectDepth->apply(state);
            else if (colormap->shaderSpheres)
                colormap->shaderSpheres->apply(state);
        } else if (lg) {
            if (colormap->shaderHeightMap)
                colormap->shaderHeightMap->apply(state);
        } else if (lighted) {
            if (colormap->shader)
                colormap->shader->apply(state);
        } else {
            if (colormap->shaderUnlit)
                colormap->shaderUnlit->apply(state);
        }
        s_coverMutex.unlock();
    } else if (!shadername.empty()) {
        s_coverMutex.lock();
        if (opencover::coVRShader *shader = opencover::coVRShaderList::instance()->get(shadername, &parammap)) {
            shader->apply(state);
        }
        s_coverMutex.unlock();
    }

    state->addUniform(new osg::Uniform("dataValid", dataValid));
    state->addUniform(new osg::Uniform("textureValid", textureValid));
#else
    (void)dataValid;
    (void)textureValid;
#endif

    int count = 0;
    for (auto d: draw) {
        if (!d.get())
            continue;
        std::string name = nodename + ".draw";
        if (draw.size() > 1)
            name += std::to_string(count);
        d->setName(name);

#ifdef COVER_PLUGIN
        opencover::cover->setRenderStrategy(d.get());

#ifdef BUILD_KDTREES
#if (OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))
        if (auto geom = d->asGeometry()) {
            osg::ref_ptr<osg::KdTreeBuilder> builder;
            std::unique_lock<std::mutex> guard(kdTreeMutex);
            if (kdTreeBuilders.empty()) {
                guard.unlock();
                builder = new osg::KdTreeBuilder;
            } else {
                builder = kdTreeBuilders.back();
                kdTreeBuilders.pop_back();
                guard.unlock();
            }
            builder->apply(*geom);
            guard.lock();
            kdTreeBuilders.push_back(builder);
        }
#endif
#endif

#endif

        geode->setStateSet(state.get());
        geode->addDrawable(d);
        ++count;
    }

    std::cerr << debug.str() << std::endl;

    return geode;
}

OsgColorMap::OsgColorMap(bool withData): texture(new osg::Texture1D), image(new osg::Image)
{
    texture->setInternalFormat(GL_RGBA8);

    texture->setBorderWidth(0);
    texture->setResizeNonPowerOfTwoHint(false);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    texture->setWrap(osg::Texture1D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);

    image->allocateImage(2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char *rgba = image->data();
    rgba[0] = 16;
    rgba[1] = 16;
    rgba[2] = 16;
    rgba[3] = 255;
    rgba[4] = 200;
    rgba[5] = 200;
    rgba[6] = 200;
    rgba[7] = 255;

    texture->setImage(image);

#ifdef COVER_PLUGIN
    //s_coverMutex.lock();
    auto parammap = get_shader_parameters();

    shader.reset(opencover::coVRShaderList::instance()->getUnique("MapColorsAttrib", &parammap));
    shaderUnlit.reset(opencover::coVRShaderList::instance()->getUnique("MapColorsAttribUnlit", &parammap));
    shaderHeightMap.reset(opencover::coVRShaderList::instance()->getUnique("MapColorsHeightmap", &parammap));
    shaderSpheres.reset(opencover::coVRShaderList::instance()->getUnique("MapColorsSpheres", &parammap));
    shaderSpheresCorrectDepth.reset(
        opencover::coVRShaderList::instance()->getUnique("MapColorsSpheres", &parammap, "#define CORRECT_DEPTH\n"));
    for (auto s: {shader, shaderUnlit, shaderHeightMap, shaderSpheres, shaderSpheresCorrectDepth}) {
        if (s)
            allShaders.push_back(s);
    }

    for (auto s: allShaders) {
        s->setFloatUniform("rangeMin", rangeMin);
        s->setFloatUniform("rangeMax", rangeMax);
        s->setBoolUniform("blendWithMaterial", blendWithMaterial);
    }
    //s_coverMutex.unlock();
#endif
}

OsgColorMap::OsgColorMap(): OsgColorMap(true)
{}

void OsgColorMap::setName(const std::string &species)
{
    texture->setName("Colormap texture: species=" + species);
    image->setName("Colormap image: species=" + species);
}

void OsgColorMap::setRange(float min, float max)
{
    rangeMin = min;
    rangeMax = max;
#ifdef COVER_PLUGIN
    for (auto s: allShaders) {
        if (s) {
            s->setFloatUniform("rangeMin", min);
            s->setFloatUniform("rangeMax", max);
        }
    }
#endif
}

void OsgColorMap::setBlendWithMaterial(bool enable)
{
    blendWithMaterial = enable;
#ifdef COVER_PLUGIN
    for (auto s: allShaders) {
        if (s) {
            if (s) {
                s->setBoolUniform("blendWithMaterial", enable);
            }
        }
    }
#endif
}
