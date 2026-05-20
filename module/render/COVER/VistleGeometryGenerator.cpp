#define _USE_MATH_DEFINES
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
#include <osg/Version>
#include "HeightMap.h"

#include <vistle/util/math.h>
#include <vistle/util/meta.h>
#include <vistle/core/polygons.h>
#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/placeholder.h>
#include <vistle/core/normals.h>
#include <vistle/core/layergrid.h>
#include <vistle/core/celltypes.h>

#include <cover/RenderObject.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRShader.h>
#include <cover/coVRPluginSupport.h>
#include <PluginUtil/Tipsify.h>

using namespace vistle;
typedef VistleGeometryGenerator::Options Options;

namespace {
const Index TileSize = 256;
const int RadiusAttrib = 11; // nvidia: gl_MultiTexCoord3
const int TfTexUnit = 0;
const int DataAttrib = 10; // nvidia: gl_MultiTexCoord2
} // namespace

std::mutex VistleGeometryGenerator::s_coverMutex;

namespace {
std::map<std::string, std::string> get_shader_parameters()
{
    std::map<std::string, std::string> parammap;
    parammap["dataAttrib"] = std::to_string(DataAttrib);
    parammap["texUnit1"] = std::to_string(TfTexUnit);
    parammap["radiusAttrib"] = std::to_string(RadiusAttrib);
    return parammap;
}

struct SphereGenerator {
    static const int MinNumLat = 4;
    static const int MinNumLong = 7;
    static_assert(MinNumLat >= 3, "too few vertices");
    static_assert(MinNumLong >= 3, "too few vertices");

    Index NumLat = MinNumLat;
    Index NumLong = MinNumLong;
    Index TriPerSphere = MinNumLong * (MinNumLat - 2) * 2;
    Index CoordPerSphere = MinNumLong * (MinNumLat - 2) + 2;

    SphereGenerator(int quality)
    : NumLat(MinNumLat + quality)
    , NumLong(MinNumLong + 2 * quality)
    , TriPerSphere(NumLong * (NumLat - 2) * 2)
    , CoordPerSphere(NumLong * (NumLat - 2) + 2)
    {}

    void addSphere(Scalar cx, Scalar cy, Scalar cz, Scalar r, Index idx, unsigned int *ti, osg::Vec3Array *vertices,
                   osg::Vec3Array *normals)
    {
        const float psi = M_PI / (NumLat - 1);
        const float phi = M_PI * 2 / NumLong;

        // create normals
        {
            Index ci = idx;
            // south pole
            normals->at(ci).set(0, 0, -1);
            ++ci;

            float Psi = -M_PI * 0.5 + psi;
            for (Index j = 0; j < NumLat - 2; ++j) {
                float Phi = j * 0.5f * phi;
                for (Index k = 0; k < NumLong; ++k) {
                    auto nx = sin(Phi) * cos(Psi);
                    auto ny = cos(Phi) * cos(Psi);
                    auto nz = sin(Psi);
                    normals->at(ci).set(nx, ny, nz);
                    ++ci;
                    Phi += phi;
                }
                Psi += psi;
            }
            // north pole
            normals->at(ci).set(0, 0, 1);
            ++ci;
            assert(ci == idx + CoordPerSphere);
        }

        // create coordinates from normals
        for (Index ci = idx; ci < idx + CoordPerSphere; ++ci) {
            vertices->at(ci) = normals->at(ci) * r + osg::Vec3(cx, cy, cz);
        }

        // create index list
        {
            Index ii = 0;
            // indices for ring around south pole
            Index ci = idx + 1;
            for (Index k = 0; k < NumLong; ++k) {
                ti[ii++] = ci + k;
                ti[ii++] = ci + (k + 1) % NumLong;
                ti[ii++] = idx;
            }

            ci = idx + 1;
            for (Index j = 0; j < NumLat - 3; ++j) {
                for (Index k = 0; k < NumLong; ++k) {
                    ti[ii++] = ci + k;
                    ti[ii++] = ci + k + NumLong;
                    ti[ii++] = ci + (k + 1) % NumLong;
                    ti[ii++] = ci + (k + 1) % NumLong;
                    ti[ii++] = ci + k + NumLong;
                    ti[ii++] = ci + NumLong + (k + 1) % NumLong;
                }
                ci += NumLong;
            }
            assert(ci == idx + 1 + NumLong * (NumLat - 3));
            assert(ci + NumLong + 1 == idx + CoordPerSphere);

            // indices for ring around north pole
            for (Index k = 0; k < NumLong; ++k) {
                ti[ii++] = ci + (k + 1) % NumLong;
                ti[ii++] = ci + k;
                ti[ii++] = idx + CoordPerSphere - 1;
            }
            assert(ii == 3 * TriPerSphere);

            for (Index j = 0; j < 3 * TriPerSphere; ++j) {
                assert(ti[j] >= idx);
                assert(ti[j] < idx + CoordPerSphere);
            }
        }
    }
};

} // namespace

template<class Geo>
struct PrimitiveAdapter {
    PrimitiveAdapter(typename Geo::const_ptr geo)
    : m_geo(geo)
    , m_numPrim(geo->getNumElements())
    , m_x(geo->x().data())
    , m_y(geo->y().data())
    , m_z(geo->z().data())
    , m_cl(m_geo->getNumCorners() > 0 ? m_geo->cl().data() : nullptr)
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
, m_x(geo->x().data())
, m_y(geo->y().data())
, m_z(geo->z().data())
, m_el(geo->el().data())
, m_cl(m_geo->getNumCorners() > 0 ? m_geo->cl().data() : nullptr)
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
        auto species = m_mapped->getAttribute(attribute::Species);
        auto source = m_mapped->getAttribute(attribute::DataSource);
        int sourceModule = source.empty() ? vistle::message::Id::Invalid : std::stoi(source);
        m_colorMapKey = vistle::ColorMapKey(species, sourceModule);
    }
}

void VistleGeometryGenerator::setOptions(const Options &options)
{
    m_options = options;
}

const vistle::ColorMapKey &VistleGeometryGenerator::colorMapKey() const
{
    return m_colorMapKey;
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
    , x(size > 0 ? mapped->x().data() : nullptr)
    , y(size > 0 ? mapped->y().data() : nullptr)
    , z(size > 0 ? mapped->z().data() : nullptr)
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
struct DataAdapter<Geometry, osg::FloatArray, const vistle::Vec<Scalar, 3>, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename vistle::Vec<Scalar, 3>::const_ptr mapped)
    : size(mapped->getSize())
    , x(size > 0 ? mapped->x().data() : nullptr)
    , y(size > 0 ? mapped->y().data() : nullptr)
    , z(size > 0 ? mapped->z().data() : nullptr)
    , mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) { return Vector3(x[idx], y[idx], z[idx]).norm(); }
    vistle::Index size = 0;
    const Scalar *x = nullptr, *y = nullptr, *z = nullptr;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, bool normalize>
struct DataAdapter<Geometry, osg::FloatArray, const vistle::Vec<Index>, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename vistle::Vec<Index>::const_ptr mapped)
    : size(mapped->getSize()), x(size > 0 ? mapped->x().data() : nullptr), mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) { return x[idx]; }
    vistle::Index size = 0;
    const Index *x = nullptr;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, class Mapped, bool normalize>
struct DataAdapter<Geometry, osg::FloatArray, Mapped, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, typename Mapped::const_ptr mapped)
    : size(mapped->getSize()), x(size > 0 ? mapped->x().data() : nullptr), mapping(mapped->guessMapping(tri))
    {}
    float getValue(Index idx) { return x[idx]; }
    vistle::Index size = 0;
    const typename Mapped::Scalar *x = nullptr;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified;
};

template<class Geometry, bool normalize>
struct DataAdapter<Geometry, osg::Vec3Array, osg::Vec3Array, normalize> {
    DataAdapter(typename Geometry::const_ptr tri, osg::Vec3Array *mapped)
    : size(mapped->size())
    , mapping(size == tri->getNumElements() ? vistle::DataBase::Element : vistle::DataBase::Vertex)
    , mapped(size > 0 ? mapped : nullptr)
    {}
    osg::Vec3 getValue(Index idx)
    {
        osg::Vec3 val = (*mapped)[idx];
        if (normalize)
            val.normalize();
        return val;
    }
    vistle::Index size = 0;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Vertex;
    osg::Vec3Array *mapped = nullptr;
};

template<class Geometry, class MappedPtr, class Array, bool normalize>
Array *applyTriangle(typename Geometry::const_ptr tri, MappedPtr mapped, const Options &options, PrimitiveBin &bin)
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
        cl = tri->cl().data();
    bool buildConn = bin.ntri == InvalidIndex;
    if (buildConn) {
        bin.ntri = 0;
        if (adap.mapping == vistle::DataBase::Vertex && options.indexedGeometry) {
            bin.vertexMap.resize(tri->getNumCoords(), InvalidIndex);
        }
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
            if (options.indexedGeometry) {
                for (Index i = begin; i < end - 2; ++i) {
                    if (buildConn)
                        ++bin.ntri;
                    for (Index k = 0; k < 3; ++k) {
                        Index v = k == 0 ? cl[begin] : cl[i + k];
                        Index &vm = bin.vertexMap[v];
                        if (buildConn) {
                            if (vm == InvalidIndex) {
                                assert(buildConn);
                                vm = vcount;
                            }
                            bin.ncl.push_back(vm);
                        }
                        assert(vm != InvalidIndex);
                        if (vm == vcount) {
                            ++vcount;
                            arr->push_back(adap.getValue(v));
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
osg::Vec3Array *computeNormals(typename Geometry::const_ptr geometry, const Options &options)
{
    PrimitiveAdapter<Geometry> geo(geometry);

    const Index numCorners = geometry->getNumCorners();
    const Index numCoords = geometry->getNumCoords();
    const Index numPrim = geometry->getNumElements();

    const Index *cl = geometry->cl().data();
    const vistle::Scalar *x = geometry->x().data();
    const vistle::Scalar *y = geometry->y().data();
    const vistle::Scalar *z = geometry->z().data();

    osg::Vec3Array *normals = new osg::Vec3Array;
    if (numCorners > 0) {
        bool useVertexNormals = options.indexedGeometry;
        normals->resize(useVertexNormals ? numCoords : numPrim);
        for (Index prim = 0; prim < numPrim; ++prim) {
            const Index begin = geo.getPrimitiveBegin(prim), end = geo.getPrimitiveBegin(prim + 1);
            assert(end - begin >= 3);
            Index v0 = cl[begin + 0], v1 = cl[begin + 1], v2 = cl[begin + 2];
            osg::Vec3 u(x[v0], y[v0], z[v0]);
            osg::Vec3 v(x[v1], y[v1], z[v1]);
            osg::Vec3 w(x[v2], y[v2], z[v2]);
            osg::Vec3 normal = (v - u) ^ (w - u);
            normal.normalize();

            if (useVertexNormals) {
                for (Index c = begin; c < end; ++c) {
                    const Index v = cl[c];
                    (*normals)[v] += normal;
                }
            } else {
                (*normals)[prim] = normal;
            }
        }
        if (useVertexNormals) {
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

osg::PrimitiveSet *buildTrianglesFromTriangles(const PrimitiveBin &bin, const Options &options, const Byte *ghost)
{
    Index numTri = bin.prim.size();
    Index numCorners = bin.ncl.size();
    if (options.indexedGeometry) {
        const Index *cl = bin.ncl.data();
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        for (Index corner = 0; corner < numCorners; corner++) {
            Index el = corner / 3;
            if (ghost && ghost[el] == cell::GHOST)
                continue;
            corners->push_back(cl[corner]);
        }
        if (options.optimizeIndices) {
            opencover::tipsify(&(*corners)[0], corners->size());
        }
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
        if (options.optimizeIndices) {
            opencover::tipsify(&(*corners)[0], corners->size());
        }
        return corners;
    } else {
        return new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, bin.ntri * 3);
    }

    return nullptr;
}

osg::PrimitiveSet *buildTrianglesFromQuads(const PrimitiveBin &bin, const Options &options, const Byte *ghost)
{
    Index numTri = bin.prim.size() * 2;
    Index numCorners = bin.ncl.size();
    if (options.indexedGeometry) {
        const Index *cl = bin.ncl.data();
        auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        corners->reserve(numTri * 3);
        for (Index corner = 0; corner < numCorners; corner++) {
            Index el = corner / 6;
            if (ghost && ghost[el] == cell::GHOST)
                continue;
            corners->push_back(cl[corner]);
        }
        if (options.optimizeIndices) {
            opencover::tipsify(&(*corners)[0], corners->size());
        }
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
        if (options.optimizeIndices) {
            opencover::tipsify(&(*corners)[0], corners->size());
        }
        return corners;
    } else {
        return new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, bin.ntri * 3);
    }

    return nullptr;
}

osg::PrimitiveSet *buildTriangles(const PrimitiveBin &bin, const Index *el, const Options &options, const Byte *ghost)
{
    Index numElements = bin.prim.size();
    Index numCorners = bin.ncl.size();
    Index numTri = numCorners - 2 * numElements;
    if (options.indexedGeometry) {
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
        if (options.optimizeIndices) {
            opencover::tipsify(&(*corners)[0], corners->size());
        }
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
        if (options.optimizeIndices) {
            opencover::tipsify(&(*corners)[0], corners->size());
        }
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
float getValue<vistle::Vec<Scalar, 3>>(typename vistle::Vec<Scalar, 3>::const_ptr data, Index idx)
{
    auto x = data->x()[idx];
    auto y = data->y()[idx];
    auto z = data->z()[idx];
    return sqrt(x * x + y * y + z * z);
}

template<class MappedObject>
osg::FloatArray *buildArray(typename MappedObject::const_ptr data, Coords::const_ptr coords, std::stringstream &debug,
                            const Options &options, std::vector<Index> *multiplicity = nullptr)
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
            cl = indexed->cl().data();
        if (multiplicity) {
            for (Index idx = 0; idx < multiplicity->size(); ++idx) {
                const Index mult = (*multiplicity)[idx];
                Index v = idx;
                if (cl)
                    v = cl[idx];
                for (Index count = 0; count < mult; ++count)
                    tc->push_back(getValue<MappedObject>(data, v));
            }
        } else if (options.indexedGeometry || !cl) {
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
            const auto el = indexed->el().data();
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
            const auto el = indexed->el().data();
            const auto numElements = indexed->getNumElements();
            if (multiplicity) {
                for (Index index = 0; index < numElements; ++index) {
                    const Index num = el[index + 1] - el[index];
                    for (Index n = 0; n < num; n++) {
                        const Index mult = (*multiplicity)[index];
                        for (Index count = 0; count < mult; ++count)
                            tc->push_back(getValue<MappedObject>(data, index));
                    }
                }
            } else {
                for (Index index = 0; index < numElements; ++index) {
                    const Index num = el[index + 1] - el[index];
                    for (Index n = 0; n < num; n++) {
                        tc->push_back(getValue<MappedObject>(data, index));
                    }
                }
            }
        } else {
            const auto numCoords = coords->getSize();
            if (multiplicity) {
                for (Index index = 0; index < numCoords; ++index) {
                    const Index mult = (*multiplicity)[index];
                    for (Index count = 0; count < mult; ++count)
                        tc->push_back(getValue<MappedObject>(data, index));
                }
            } else {
                for (Index index = 0; index < numCoords; ++index) {
                    tc->push_back(getValue<MappedObject>(data, index));
                }
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

const OsgColorMap *VistleGeometryGenerator::getColorMap(const vistle::ColorMapKey &key) const
{
    std::lock_guard<std::mutex> lock(s_coverMutex);
    if (m_colormaps) {
        auto it = m_colormaps->find(key);
        if (it != m_colormaps->end()) {
            return &it->second;
        }
    }
    return nullptr;
}

std::string getAttribute(const std::string &name, const vistle::Object::const_ptr data,
                         const vistle::Object::const_ptr grid = nullptr)
{
    if (data && data->hasAttribute(name))
        return data->getAttribute(name);
    if (grid && grid->hasAttribute(name))
        return grid->getAttribute(name);
    return "";
}

osg::Material *createSolidMaterial(const std::string &name, const vistle::Vector4 &color)
{
    osg::Material *material = new osg::Material;
    material->setName(name + ".material");
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(color[0], color[1], color[2], color[3]));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(color[0], color[1], color[2], color[3]));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2, 0.2, 0.2, color[3]));
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    return material;
}

template<template<typename> class Wrapper, typename Tuple>
struct TransformTuple;

template<template<typename> class Wrapper, typename... Ts>
struct TransformTuple<Wrapper, std::tuple<Ts...>> {
    using type = std::tuple<Wrapper<Ts>...>;
};

template<template<typename> class Wrapper, typename Tuple>
using TransformTuple_t = typename TransformTuple<Wrapper, Tuple>::type;

using VecTypes = std::tuple<vistle::Vec<Scalar>, vistle::Vec<Scalar, 3>, vistle::Vec<Index>, vistle::Vec<Byte>>;


VistleGeometryGenerator::NodeInfo VistleGeometryGenerator::gatherNodeInfo()
{
    NodeInfo info;
    info.nodename = m_geo->getName();

    std::stringstream &debug = info.debug;
    debug << info.nodename << " ";
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


    std::vector<osg::ref_ptr<osg::Drawable>> draw;
    auto &state = info.state;

    bool transparent = false;
    bool forceOpaque = false;
    if (auto t = getAttribute(attribute::Transparent, m_mapped, m_geo); !t.empty()) {
        transparent = t != "false";
        forceOpaque = !transparent;
    }

    osg::Material *material = nullptr;
    if (m_ro && m_ro->hasSolidColor) {
        const auto &c = m_ro->solidColor;
        material = createSolidMaterial(info.nodename, c);
        if (c[3] > 0.f && c[3] < 1.f)
            transparent = true;
    }

    if (forceOpaque) {
        transparent = false;
    }
    if (transparent) {
        state = opencover::VRSceneGraph::instance()->loadTransparentGeostate();
    } else {
        state = opencover::VRSceneGraph::instance()->loadDefaultGeostate();
    }
    if (!state) {
        state = new osg::StateSet;
        if (transparent) {
            state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            state->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
    }
    if (auto depthOnly = getAttribute(attribute::DepthOnly, m_mapped, m_geo); !depthOnly.empty()) {
        state->setRenderBinDetails(-1, "RenderBin");
        state->setNestRenderBins(false);
        state->setAttributeAndModes(new osg::ColorMask(false, false, false, false), osg::StateAttribute::ON);
    }

    state->setName(info.nodename + ".state");
    if (material)
        state->setAttribute(material);

    info.normals = vistle::Normals::as(m_normal);
    if (info.normals) {
        auto m = info.normals->guessMapping(m_geo);
        if (m != vistle::DataBase::Vertex) {
            m_options.indexedGeometry = false;
            info.debug << "NoIndex: normals ";
        }
    }

    vistle::DataBase::const_ptr database = vistle::DataBase::as(m_mapped);

    if (database) {
        info.mapping = database->guessMapping(m_geo);
        if (info.mapping != vistle::DataBase::Vertex) {
            m_options.indexedGeometry = false;
            info.debug << "NoIndex: data ";
        }
    }

    if (m_mapped && meta::dispatch_by_type<VecTypes>(*m_mapped, []<typename T>() {})) {
        info.colormap = getColorMap(m_colorMapKey);
    } else if (database) {
        info.debug << "Unsupported mapped data: type=" << Object::toString(database->getType()) << " ("
                   << database->getType() << ")";
    }

    info.numPrimitives = m_options.numPrimitives;
    if (auto np = getAttribute(attribute::BinNumPrimitives, m_mapped, m_geo); !np.empty()) {
        info.numPrimitives = atol(np.c_str());
    }

    return info;
}

void addFromCache(osg::Geometry *geom, GeometryCache &cache, unsigned nbin)
{
    std::unique_lock<GeometryCache> guard(cache);
    geom->setVertexArray(cache.vertices[nbin]);
    geom->addPrimitiveSet(cache.primitives[nbin]);
    geom->setNormalArray(cache.normals[nbin]);
}

void storeInCache(GeometryCache &cache, osg::Vec3Array *vertices, osg::PrimitiveSet *ps, osg::Vec3Array *normals)
{
    // std::unique_lock<GeometryCache> guard(cache);
    cache.vertices.push_back(vertices);
    cache.primitives.push_back(ps);
    cache.normals.push_back(normals);
}

osg::Drawable *VistleGeometryGenerator::handlePoints(NodeInfo &info)
{
    m_options.indexedGeometry = false;

    vistle::Points::const_ptr points = vistle::Points::as(m_geo);
    assert(points);
    const Index numVertices = points->getNumPoints();

    auto radius = points->radius();

    info.debug << "Points: [ #v " << numVertices << (radius ? " with radius" : "") << " ]";
    const vistle::Scalar *x = points->x().data();
    const vistle::Scalar *y = points->y().data();
    const vistle::Scalar *z = points->z().data();

    info.state->setAttribute(new osg::Point(2.0f), osg::StateAttribute::ON);


    auto geom = new osg::Geometry();

    if (numVertices > 0) {
        if (info.cached) {
            std::unique_lock<GeometryCache> guard(info.cache);
            geom->setVertexArray(info.cache.vertices.front());
            geom->addPrimitiveSet(info.cache.primitives.front());
        } else {
            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
            for (Index v = 0; v < numVertices; v++)
                vertices->push_back(osg::Vec3(x[v], y[v], z[v]));

            geom->setVertexArray(vertices.get());
            auto ps = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, numVertices);
            geom->addPrimitiveSet(ps);
            if (m_cache) {
                info.cache.vertices.push_back(vertices);
                info.cache.primitives.push_back(ps);
            }
        }

        if (radius) {
            info.haveSpheres = true;
            const vistle::Scalar *r = radius->x().data();

            osg::ref_ptr<osg::FloatArray> rad = new osg::FloatArray();
            rad->reserve(numVertices);
            for (Index v = 0; v < numVertices; v++)
                rad->push_back(r[v]);
            geom->setVertexAttribArray(RadiusAttrib, rad, osg::Array::BIND_PER_VERTEX);

            geom->setUseDisplayList(false);
            geom->setSupportsDisplayList(false);
            geom->setUseVertexBufferObjects(true);

            if (!info.colormap) {
                // required for applying shader
                info.colormap = getColorMap(ColorMapKey());
            }
        } else {
            info.lighted = false;
        }
    }
    return geom;
}

std::vector<osg::ref_ptr<osg::Drawable>> VistleGeometryGenerator::handleLayerGrid(NodeInfo &info)
{
    static_assert(TileSize >= 4,
                  "TileSize needs to be at least four (start and end border and repeated border vertices)");
    vistle::LayerGrid::const_ptr lg = vistle::LayerGrid::as(m_geo);
    const Index numVertices = lg->getNumVertices();

    info.debug << "LayerGrid: [ #v " << numVertices << " (" << lg->getNumDivisions(0) << " " << lg->getNumDivisions(1)
               << " " << lg->getNumDivisions(2) << ") ]";

    Index dims[3] = {
        lg->getNumDivisions(0),
        lg->getNumDivisions(1),
        lg->getNumDivisions(2),
    };

    HeightMap::DataMode dmode = HeightMap::NoData;
    if (info.mapping == DataBase::Vertex)
        dmode = HeightMap::VertexData;
    else if (info.mapping == DataBase::Element)
        dmode = HeightMap::CellData;

    if (dmode == HeightMap::NoData) {
        info.colormap = getColorMap(ColorMapKey());
    } else {
        info.dataValid = true;
    }

    const auto dx = lg->dist()[0];
    const auto dy = lg->dist()[1];
    std::vector<osg::ref_ptr<osg::Drawable>> draw;
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

                fillTexture<LayerGrid>(info.debug, height, sx, sy, lg, dims, x, y, l);
                if (m_mapped) {
                    meta::dispatch_by_type<VecTypes>(*m_mapped, [&]<typename T>() {
                        fillTexture<T>(info.debug, data, sx, sy, std::static_pointer_cast<const T>(m_mapped), dims, x,
                                       y, l);
                    });
                }

                hf->dirty();
                hf->build();
                draw.push_back(hf);

                borderWidth[0] = borderWidth[1];
            }

            borderWidth[2] = borderWidth[3];
        }
    }
    m_options.indexedGeometry = false;
    return draw;
}

std::vector<osg::ref_ptr<osg::Drawable>> VistleGeometryGenerator::handleTriangles(NodeInfo &info)
{
    vistle::Triangles::const_ptr triangles = vistle::Triangles::as(m_geo);

    const Index numCorners = triangles->getNumCorners();
    const Index numVertices = triangles->getNumVertices();
    if (numCorners == 0) {
        m_options.indexedGeometry = false;
    }
    const Byte *ghost = nullptr;
    if (triangles->ghost().size() > 0)
        ghost = triangles->ghost().data();

    info.debug << "Triangles: [ #c " << numCorners << ", #v " << numVertices
               << ", indexed=" << (m_options.indexedGeometry ? "t" : "f") << " ]";

    osg::ref_ptr<osg::Vec3Array> gnormals;
    if (!info.cached && !info.normals)
        gnormals = computeNormals<vistle::Triangles>(triangles, m_options);

    auto bins = binPrimitives<vistle::Triangles>(triangles, info.numPrimitives);
    info.debug << " #bins: " << bins.size();
    std::vector<osg::ref_ptr<osg::Drawable>> draw;
    unsigned nbin = 0;
    for (auto bin: bins) {
        auto geom = new osg::Geometry();
        draw.push_back(geom);

        auto &cache = info.cache;
        if (info.cached) {
            addFromCache(geom, cache, nbin);
        } else {
            osg::ref_ptr<osg::Vec3Array> vertices =
                applyTriangle<Triangles, Triangles::const_ptr, osg::Vec3Array, false>(triangles, triangles, m_options,
                                                                                      bin);
            geom->setVertexArray(vertices);

            auto ps = buildTrianglesFromTriangles(bin, m_options, ghost);
            geom->addPrimitiveSet(ps);

            osg::ref_ptr<osg::Vec3Array> norm = applyTriangle<Triangles, Normals::const_ptr, osg::Vec3Array, true>(
                triangles, info.normals, m_options, bin);
            if (!norm)
                norm = applyTriangle<Triangles, osg::Vec3Array *, osg::Vec3Array, false>(triangles, gnormals.get(),
                                                                                         m_options, bin);
            geom->setNormalArray(norm.get());
            storeInCache(cache, vertices, ps, norm);
        }
        osg::ref_ptr<osg::FloatArray> fl;

        if (m_mapped) {
            meta::dispatch_by_type<VecTypes>(*m_mapped, [&]<typename T>() {
                fl = applyTriangle<Triangles, typename T::const_ptr, osg::FloatArray, false>(
                    triangles, std::static_pointer_cast<const T>(m_mapped), m_options, bin);
            });
        }

        if (fl) {
            geom->setVertexAttribArray(DataAttrib, fl);
            info.dataValid = true;
        }

        bin.clear();
        ++nbin;
    }
    return draw;
}

std::vector<osg::ref_ptr<osg::Drawable>> VistleGeometryGenerator::handleQuads(NodeInfo &info)
{
    vistle::Quads::const_ptr quads = vistle::Quads::as(m_geo);

    const Index numCorners = quads->getNumCorners();
    const Index numVertices = quads->getNumVertices();
    if (numCorners == 0) {
        m_options.indexedGeometry = false;
    }
    const Byte *ghost = nullptr;
    if (quads->ghost().size() > 0)
        ghost = quads->ghost().data();

    info.debug << "Quads: [ #c " << numCorners << ", #v " << numVertices
               << ", indexed=" << (m_options.indexedGeometry ? "t" : "f") << " ]";

    osg::ref_ptr<osg::Vec3Array> gnormals;
    if (!info.cached && !info.normals)
        gnormals = computeNormals<vistle::Quads>(quads, m_options);

    auto bins = binPrimitives<vistle::Quads>(quads, info.numPrimitives);
    info.debug << " #bins: " << bins.size();

    unsigned nbin = 0;
    std::vector<osg::ref_ptr<osg::Drawable>> draw;
    for (auto bin: bins) {
        auto geom = new osg::Geometry();
        draw.push_back(geom);
        if (info.cached) {
            addFromCache(geom, info.cache, nbin);
        } else {
            osg::ref_ptr<osg::Vec3Array> vertices =
                applyTriangle<Quads, Quads::const_ptr, osg::Vec3Array, false>(quads, quads, m_options, bin);
            geom->setVertexArray(vertices);

            auto ps = buildTrianglesFromQuads(bin, m_options, ghost);
            geom->addPrimitiveSet(ps);

            osg::ref_ptr<osg::Vec3Array> norm =
                applyTriangle<Quads, Normals::const_ptr, osg::Vec3Array, true>(quads, info.normals, m_options, bin);
            if (!norm)
                norm = applyTriangle<Quads, osg::Vec3Array *, osg::Vec3Array, false>(quads, gnormals.get(), m_options,
                                                                                     bin);
            geom->setNormalArray(norm.get());
            storeInCache(info.cache, vertices, ps, norm);
        }

        osg::ref_ptr<osg::FloatArray> fl;
        if (m_mapped) {
            meta::dispatch_by_type<VecTypes>(*m_mapped, [&]<typename T>() {
                fl = applyTriangle<Quads, typename T::const_ptr, osg::FloatArray, false>(
                    quads, std::static_pointer_cast<const T>(m_mapped), m_options, bin);
            });
        }

        if (fl) {
            geom->setVertexAttribArray(DataAttrib, fl);
            info.dataValid = true;
        }

        bin.clear();
        ++nbin;
    }
    return draw;
}


std::vector<osg::ref_ptr<osg::Drawable>> VistleGeometryGenerator::handlePolygons(NodeInfo &info)
{
    vistle::Polygons::const_ptr polygons = vistle::Polygons::as(m_geo);

    const Index numElements = polygons->getNumElements();
    const Index numCorners = polygons->getNumCorners();
    const Index numVertices = polygons->getNumVertices();
    if (numCorners == 0) {
        m_options.indexedGeometry = false;
    }
    const Byte *ghost = nullptr;
    if (polygons->ghost().size() > 0)
        ghost = polygons->ghost().data();

    info.debug << "Polygons: [ #c " << numCorners << ", #e " << numElements << ", #v " << numVertices
               << ", indexed=" << (m_options.indexedGeometry ? "t" : "f") << " ]";

    const Index *el = polygons->el().data();

    osg::ref_ptr<osg::Vec3Array> gnormals;
    if (!info.cached && !info.normals)
        gnormals = computeNormals<vistle::Indexed>(polygons, m_options);

    auto bins = binPrimitives<vistle::Indexed>(polygons, info.numPrimitives);
    info.debug << " #bins: " << bins.size();

    std::vector<osg::ref_ptr<osg::Drawable>> draw;
    unsigned nbin = 0;
    for (auto bin: bins) {
        auto geom = new osg::Geometry();
        draw.push_back(geom);

        if (info.cached) {
            addFromCache(geom, info.cache, nbin);
        } else {
            osg::ref_ptr<osg::Vec3Array> vertices =
                applyTriangle<Indexed, Polygons::const_ptr, osg::Vec3Array, false>(polygons, polygons, m_options, bin);
            geom->setVertexArray(vertices);

            auto ps = buildTriangles(bin, el, m_options, ghost);
            geom->addPrimitiveSet(ps);

            osg::ref_ptr<osg::Vec3Array> norm = applyTriangle<Indexed, Normals::const_ptr, osg::Vec3Array, true>(
                polygons, info.normals, m_options, bin);
            if (!norm)
                norm = applyTriangle<Indexed, osg::Vec3Array *, osg::Vec3Array, false>(polygons, gnormals.get(),
                                                                                       m_options, bin);
            geom->setNormalArray(norm.get());
            storeInCache(info.cache, vertices, ps, norm);
        }

        osg::ref_ptr<osg::FloatArray> fl;
        if (m_mapped) {
            meta::dispatch_by_type<VecTypes>(*m_mapped, [&]<typename T>() {
                fl = applyTriangle<Indexed, typename T::const_ptr, osg::FloatArray, false>(
                    polygons, std::static_pointer_cast<const T>(m_mapped), m_options, bin);
            });
        }
        if (fl) {
            geom->setVertexAttribArray(DataAttrib, fl);
            info.dataValid = true;
        }

        bin.clear();
        ++nbin;
    }
    return draw;
}

osg::Drawable *VistleGeometryGenerator::handleLines(NodeInfo &info)
{
    vistle::Lines::const_ptr lines = vistle::Lines::as(m_geo);
    const Index numElements = lines->getNumElements();
    const Index numCorners = lines->getNumCorners();

    auto radius = lines->radius();

    info.debug << "Lines: [ #c " << numCorners << ", #e " << numElements << (radius ? " with radius" : "") << " ]";

    m_options.indexedGeometry = false;
    auto geom = new osg::Geometry();

    if (info.cached) {
        std::unique_lock<GeometryCache> guard(info.cache);
        geom->setVertexArray(info.cache.vertices.front());
        geom->addPrimitiveSet(info.cache.primitives.front());
    } else {
        const Index *el = lines->el().data();
        const Index *cl = numCorners > 0 ? lines->cl().data() : nullptr;
        const vistle::Scalar *x = lines->x().data();
        const vistle::Scalar *y = lines->y().data();
        const vistle::Scalar *z = lines->z().data();

        osg::ref_ptr<osg::PrimitiveSet> primitives;
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        if (radius) {
            m_options.indexedGeometry = true;
            auto r = radius->x().data();
            bool radiusPerElement = radius->guessMapping(m_geo) == DataBase::Element;

            int quality = 2;
            SphereGenerator gen(quality);

            const unsigned MinNumSect = 3;
            static_assert(MinNumSect >= 3, "too few sectors");
            unsigned NumSect = MinNumSect + quality;
            Index TriPerSection = NumSect * 2;
            Index TriPerSphere = gen.TriPerSphere;
            Index CoordPerSphere = gen.CoordPerSphere;

            Index numEl = lines->getNumElements();
            Index numEmptyEl = 0, numSinglePointEl = 0;
            auto el = lines->el().data();
            for (Index i = 0; i < numEl; ++i) {
                const Index begin = el[i], end = el[i + 1];
                if (end == begin) {
                    ++numEmptyEl;
                } else if (end == begin + 1) {
                    ++numSinglePointEl;
                }
            }
            Index numPoint = lines->getNumCoords();
            Index numConn = lines->getNumCorners();
            if (numEl == 0 || numEl == numEmptyEl) {
                numPoint = 0;
                numConn = 0;
            } else if (numConn == 0) {
                numConn = numPoint;
            } else {
                cl = lines->cl().data();
            }
            // we ignore connection style altogether and simplify start and end style
            auto startStyle = lines->startStyle();
            if (startStyle != Lines::Open) {
                startStyle = Lines::Flat;
            }
            auto endStyle = lines->endStyle();
            if (endStyle != Lines::Arrow && endStyle != Lines::Open) {
                endStyle = Lines::Flat;
            }
            if (numEl == 0 || numConn == 0 || numPoint == 0) {
                startStyle = Lines::Open;
                endStyle = Lines::Open;
            }

            Index numCoordStart = 0, numCoordEnd = 0;
            Index numIndStart = 0, numIndEnd = 0;
            if (startStyle == Lines::Flat) {
                numCoordStart = 1 + NumSect;
                numIndStart = 3 * NumSect;
            }
            if (endStyle == Lines::Arrow) {
                numCoordEnd = 3 * NumSect;
                numIndEnd = 3 * 3 * NumSect;
            } else if (endStyle == Lines::Flat) {
                numCoordEnd = 1 + NumSect;
                numIndEnd = 3 * NumSect;
            }

            const Index numSeg = numConn - (numEl - numEmptyEl);
            const Index numVert = numSeg * 3 * TriPerSection +
                                  (numEl - numEmptyEl - numSinglePointEl) * (numIndStart + numIndEnd) +
                                  numSinglePointEl * 3 * TriPerSphere;
            const Index numCoord = numVert > 0
                                       ? (numConn - numSinglePointEl) * NumSect +
                                             (numEl - numEmptyEl - numSinglePointEl) * (numCoordStart + numCoordEnd) +
                                             numSinglePointEl * CoordPerSphere
                                       : 0;

            osg::ref_ptr<osg::Vec3Array> gnormals = new osg::Vec3Array(numCoord);
            auto corners = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, numVert);
            primitives = corners;
            vertices->resize(numCoord);
            vistle::DataBase::const_ptr database = vistle::DataBase::as(m_mapped);

            if (database) {
                info.multiplicity = std::make_unique<std::vector<Index>>();
                info.multiplicity->reserve(numConn);
            }
            unsigned int *ti = nullptr;
            if (numElements > 0)
                ti = &(*corners)[0];

            Index ci = 0; // coord index
            Index ii = 0; // index index
            for (Index i = 0; i < numEl; ++i) {
                const Index begin = el[i], end = el[i + 1];
                Vector3 normal, dir;
                for (Index k = begin; k < end; ++k) {
                    Index idx = cl ? cl[k] : k;
                    Index nidx = (cl && k + 1 < end) ? cl[k + 1] : k + 1;
                    Index pidx = (cl && k > 0) ? cl[k - 1] : k - 1;
                    auto curRad = radiusPerElement ? r[i] : r[idx];
                    Vector3 cur(x[idx], y[idx], z[idx]);

                    if (end == begin + 1) {
                        // single point element: draw a sphere
                        gen.addSphere(cur[0], cur[1], cur[2], curRad, ci, &ti[ii], vertices, gnormals);
                        ci += gen.CoordPerSphere;
                        ii += 3 * gen.TriPerSphere;

                        if (info.multiplicity) {
                            info.multiplicity->push_back(gen.CoordPerSphere);
                        }
                        break;
                    }

                    Vector3 next = k + 1 < end ? Vector3(x[nidx], y[nidx], z[nidx]) : cur;
                    Vector3 l1 = next - cur;
                    auto len1 = l1.norm(), len2 = Scalar();
                    bool first = false, last = false;
                    if (k == begin) {
                        first = true;
                        dir = l1.normalized();
                    } else if (k + 1 == end) {
                        last = true;
                        // keep previous direction for final segment
                    } else {
                        Vector3 l2(x[idx] - x[pidx], y[idx] - y[pidx], z[idx] - z[pidx]);
                        len2 = l2.norm();
                        if (len2 > 100 * len1) {
                            dir = l2.normalized();
                        } else if (len1 > 100 * len2) {
                            dir = l1.normalized();
                        } else {
                            dir = (l1.normalized() + l2.normalized()).normalized();
                        }
                    }

                    if (info.multiplicity) {
                        Index ncoord = NumSect;
                        if (first) {
                            ncoord += numCoordStart;
                        }
                        if (last) {
                            ncoord += numCoordEnd;
                        }
                        info.multiplicity->push_back(ncoord);
                    }

                    if (first || normal.norm() < Scalar(0.5)) {
                        normal = dir.cross(Vector3(0, 0, 1)).normalized();
                        if (normal.norm() < Scalar(0.5)) {
                            // try another direction
                            normal = dir.cross(Vector3(0, 1, 0)).normalized();
                        }
                    } else if (len1 > 1e-4 || len2 > 1e-4) {
                        normal = (normal - dir.dot(normal) * dir).normalized();
                    }

                    Quaternion qrot(AngleAxis(2. * M_PI / NumSect, dir));
                    const auto rot = qrot.toRotationMatrix();
                    const auto rot2 = Quaternion(AngleAxis(M_PI / NumSect, dir)).toRotationMatrix();

                    // start cap
                    if (first && startStyle == Lines::Flat) {
                        vertices->at(ci) = osg::Vec3(cur[0], cur[1], cur[2]);
                        gnormals->at(ci) = osg::Vec3(dir[0], dir[1], dir[2]);
                        ++ci;

                        for (Index l = 0; l < NumSect; ++l) {
                            ti[ii++] = ci - 1;
                            ti[ii++] = ci + l;
                            ti[ii++] = ci + (l + 1) % NumSect;
                        }

                        Vector3 rad = normal;
                        for (Index l = 0; l < NumSect; ++l) {
                            gnormals->at(ci) = osg::Vec3(dir[0], dir[1], dir[2]);
                            Vector3 p = cur + curRad * rad;
                            rad = rot * rad;
                            vertices->at(ci) = osg::Vec3(p[0], p[1], p[2]);
                            ++ci;
                        }
                    }

                    // indices
                    if (!last) {
                        for (Index l = 0; l < NumSect; ++l) {
                            ti[ii++] = ci + l;
                            ti[ii++] = ci + (l + 1) % NumSect;
                            ti[ii++] = ci + (l + 1) % NumSect + NumSect;
                            ti[ii++] = ci + l;
                            ti[ii++] = ci + (l + 1) % NumSect + NumSect;
                            ti[ii++] = ci + l + NumSect;
                        }
                    }

                    // coordinates and normals
                    auto n = normal;
                    for (Index l = 0; l < NumSect; ++l) {
                        gnormals->at(ci) = osg::Vec3(n[0], n[1], n[2]);
                        Vector3 p = cur + curRad * n;
                        vertices->at(ci) = osg::Vec3(p[0], p[1], p[2]);
                        n = rot * n;
                        ++ci;
                    }

                    // end cap/arrow
                    if (last && endStyle != Lines::Open) {
                        if (endStyle == Lines::Arrow) {
                            Index tipStart = ci;
                            for (Index l = 0; l < NumSect; ++l) {
                                vertices->at(ci) = vertices->at(ci - NumSect);
                                gnormals->at(ci) = osg::Vec3(dir[0], dir[1], dir[2]);
                                ++ci;
                            }

                            Scalar tipSize = 2.0;

                            Vector3 n = normal;
                            Vector3 tip = cur + tipSize * dir * curRad;
                            for (Index l = 0; l < NumSect; ++l) {
                                Vector3 norm = (n + dir).normalized();
                                Vector3 p = cur + tipSize * curRad * n;
                                n = rot * n;

                                gnormals->at(ci) = osg::Vec3(norm[0], norm[1], norm[2]);
                                vertices->at(ci) = osg::Vec3(p[0], p[1], p[2]);
                                ++ci;
                            }

                            n = rot2 * normal;
                            for (Index l = 0; l < NumSect; ++l) {
                                Vector3 norm = (n + dir).normalized();
                                n = rot * n;

                                gnormals->at(ci) = osg::Vec3(norm[0], norm[1], norm[2]);
                                vertices->at(ci) = osg::Vec3(tip[0], tip[1], tip[2]);
                                ++ci;
                            }

                            for (Index l = 0; l < NumSect; ++l) {
                                ti[ii++] = tipStart + l;
                                ti[ii++] = tipStart + (l + 1) % NumSect;
                                ti[ii++] = tipStart + NumSect + (l + 1) % NumSect;

                                ti[ii++] = tipStart + NumSect + (l + 1) % NumSect;
                                ti[ii++] = tipStart + NumSect + l;
                                ti[ii++] = tipStart + l;

                                ti[ii++] = tipStart + NumSect + l;
                                ti[ii++] = tipStart + NumSect + (l + 1) % NumSect;
                                ti[ii++] = tipStart + 2 * NumSect + l;
                            }
                        } else if (endStyle == Lines::Flat) {
                            for (Index l = 0; l < NumSect; ++l) {
                                vertices->at(ci) = vertices->at(ci - NumSect);
                                gnormals->at(ci) = osg::Vec3(dir[0], dir[1], dir[2]);
                                ++ci;
                            }

                            vertices->at(ci) = osg::Vec3(cur[0], cur[1], cur[2]);
                            gnormals->at(ci) = osg::Vec3(dir[0], dir[1], dir[2]);
                            for (Index l = 0; l < NumSect; ++l) {
                                ti[ii++] = ci - NumSect + l;
                                ti[ii++] = ci - NumSect + (l + 1) % NumSect;
                                ti[ii++] = ci;
                            }
                            ++ci;
                        }
                    }
                }
            }
            assert(ci == numCoord);
            assert(ii == numVert);
            assert(!info.multiplicity || info.multiplicity->size() == numConn);

            geom->setNormalArray(gnormals);
            geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

        } else {
            auto strips = new osg::DrawArrayLengths(osg::PrimitiveSet::LINE_STRIP);
            primitives = strips;

            for (Index index = 0; index < numElements; index++) {
                Index start = el[index];
                Index end = el[index + 1];
                Index num = end - start;

                strips->push_back(num);

                for (Index n = 0; n < num; n++) {
                    Index v = start + n;
                    if (cl)
                        v = cl[v];
                    vertices->push_back(osg::Vec3(x[v], y[v], z[v]));
                }
            }
            info.lighted = false;
            info.state->setAttributeAndModes(new osg::LineWidth(2.f), osg::StateAttribute::ON);
        }

        geom->setVertexArray(vertices.get());
        info.cache.vertices.push_back(vertices);

        geom->addPrimitiveSet(primitives.get());
        info.cache.primitives.push_back(primitives);

        if (m_ro->hasSolidColor) {
            const auto &c = m_ro->solidColor;
            osg::Vec4Array *colArray = new osg::Vec4Array();
            colArray->push_back(osg::Vec4(c[0], c[1], c[2], c[3]));
            colArray->setBinding(osg::Array::BIND_OVERALL);
            geom->setColorArray(colArray);
        }
    }
    return geom;
}

void VistleGeometryGenerator::applyColorMap(NodeInfo &info)
{
    auto &colormap = info.colormap;
    auto &state = info.state;

    if (info.dataValid) {
        info.state->setTextureAttributeAndModes(TfTexUnit, colormap->texture, osg::StateAttribute::ON);
    }
    std::lock_guard<std::mutex> guard(s_coverMutex);
    if (info.haveSpheres) {
        bool correctDepth = true;
        if (m_geo && m_geo->hasAttribute(attribute::ApproximateDepth)) {
            correctDepth = m_geo->getAttribute(attribute::ApproximateDepth) != "true";
        }
        if (correctDepth && colormap->shaderSpheresCorrectDepth)
            colormap->shaderSpheresCorrectDepth->apply(state);
        else if (colormap->shaderSpheres)
            colormap->shaderSpheres->apply(state);
    } else if (vistle::LayerGrid::as(m_geo)) {
        if (colormap->shaderHeightMap)
            colormap->shaderHeightMap->apply(state);
    } else if (info.lighted) {
        if (colormap->shader)
            colormap->shader->apply(state);
    } else {
        if (colormap->shaderUnlit)
            colormap->shaderUnlit->apply(state);
    }
}

osg::Geode *makeGeode(const std::string &nodename, const std::vector<osg::ref_ptr<osg::Drawable>> &draw,
                      osg::StateSet *state, bool buildKdTree)
{
    osg::Geode *geode = new osg::Geode();
    geode->setName(nodename);
    int count = 0;
    for (auto d: draw) {
        if (!d.get())
            continue;
        std::string name = nodename + ".draw";
        if (draw.size() > 1)
            name += std::to_string(count);
        d->setName(name);

        opencover::cover->setRenderStrategy(d.get());

#if (OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))
        if (buildKdTree) {
            if (auto geom = d->asGeometry()) {
                auto builder = new osg::KdTreeBuilder;
                builder->apply(*geom);
            }
        }
#endif

        geode->setStateSet(state);
        geode->addDrawable(d);
        ++count;
    }
    return geode;
}

std::map<std::string, std::string> setShaderParameters(const std::string &shadername, const std::string &shaderparams,
                                                       std::stringstream &debug)
{
    std::map<std::string, std::string> parammap;
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
    return parammap;
}

osg::Geode *VistleGeometryGenerator::makeNode()
{
    if (m_ro)
        m_ro->updateBounds();

    if (!m_geo)
        return nullptr;


    auto info = gatherNodeInfo();
    ResultCache<GeometryCache>::Entry *cacheEntry = nullptr;
    if (m_cache) {
        cacheEntry = m_cache->getOrLock(m_geo->getName(), info.cache);
        if (!cacheEntry)
            info.cached = true;
        if (info.cached)
            info.debug << "cached ";
    }

    std::vector<osg::ref_ptr<osg::Drawable>> draw;

    switch (m_geo->getType()) {
    case vistle::Object::PLACEHOLDER: {
        vistle::PlaceHolder::const_ptr ph = vistle::PlaceHolder::as(m_geo);
        info.debug << "Placeholder [" << ph->originalName() << "]";
        if (isSupported(ph->originalType())) {
            info.nodename = ph->originalName();
        }
        break;
    }
    case vistle::Object::POINTS: {
        draw.push_back(handlePoints(info));
        break;
    }
    case vistle::Object::LAYERGRID: {
        auto lg = handleLayerGrid(info);
        draw.insert(draw.end(), lg.begin(), lg.end());
        break;
    }

    case vistle::Object::TRIANGLES: {
        auto tri = (handleTriangles(info));
        draw.insert(draw.end(), tri.begin(), tri.end());
        break;
    }

    case vistle::Object::QUADS: {
        auto quads = handleQuads(info);
        draw.insert(draw.end(), quads.begin(), quads.end());

        break;
    }

    case vistle::Object::POLYGONS: {
        auto polys = handlePolygons(info);
        draw.insert(draw.end(), polys.begin(), polys.end());

        break;
    }

    case vistle::Object::LINES: {
        draw.push_back(handleLines(info));
        break;
    }

    default:
        assert(isSupported(m_geo->getType()) == false);
        break;
    }

    if (m_cache && !info.cached) {
        m_cache->storeAndUnlock(cacheEntry, info.cache);
    }


    info.state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    info.state->setMode(GL_LIGHTING, info.lighted ? osg::StateAttribute::ON : osg::StateAttribute::OFF);


    if (Coords::const_ptr coords = Coords::as(m_geo); coords && m_mapped && !info.dataValid && !draw.empty()) {
        if (auto geom = draw[0]->asGeometry()) {
            osg::ref_ptr<osg::FloatArray> fl;
            meta::dispatch_by_type<VecTypes>(*m_mapped, [&]<typename T>() {
                fl = buildArray<T>(std::static_pointer_cast<const T>(m_mapped), coords, info.debug, m_options,
                                   info.multiplicity.get());
            });
            if (fl && !fl->empty()) {
                //std::cerr << "VistleGeometryGenerator: setting VertexAttribArray for " << typeid(T).name() << " of size " << fl->size() << std::endl;
                geom->setVertexAttribArray(DataAttrib, fl, osg::Array::BIND_PER_VERTEX);
                info.dataValid = true;
            }
        }
    }

    if (info.colormap) {
        applyColorMap(info);
    } else if (auto shadername = m_geo->getAttribute(attribute::Shader); !shadername.empty()) {
        auto parammap = setShaderParameters(shadername, m_geo->getAttribute(attribute::ShaderParams), info.debug);
        std::lock_guard<std::mutex> guard(s_coverMutex);
        if (opencover::coVRShader *shader = opencover::coVRShaderList::instance()->get(shadername, &parammap)) {
            shader->apply(info.state);
        }
    }

    info.state->addUniform(new osg::Uniform("dataValid", info.dataValid));

    info.debug << ", colormap: " << (info.colormap ? info.colormap->species : "NO")
               << ", dataValid: " << (info.dataValid ? "true" : "false");


    std::cerr << info.debug.str() << std::endl;
    return makeGeode(info.nodename, draw, info.state.get(), m_options.buildKdTree);
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
}

OsgColorMap::OsgColorMap(): OsgColorMap(true)
{}

void OsgColorMap::setName(const std::string &species)
{
    this->species = species;
    texture->setName("Colormap texture: species=" + species);
    image->setName("Colormap image: species=" + species);
}

void OsgColorMap::setRange(float min, float max)
{
    rangeMin = min;
    rangeMax = max;
    for (auto s: allShaders) {
        if (s) {
            s->setFloatUniform("rangeMin", min);
            s->setFloatUniform("rangeMax", max);
        }
    }
}

void OsgColorMap::setBlendWithMaterial(bool enable)
{
    blendWithMaterial = enable;
    for (auto s: allShaders) {
        if (s) {
            if (s) {
                s->setBoolUniform("blendWithMaterial", enable);
            }
        }
    }
}
