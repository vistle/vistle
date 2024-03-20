#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/lines.h>
#include <vistle/core/tubes.h>
#include <vistle/core/spheres.h>
#include <vistle/core/points.h>
#include <vistle/core/vec.h>

#include <cassert>

#include "anarirenderobject.h"
#include <anari/anari_cpp/ext/std.h>

using namespace vistle;

float AnariRenderObject::pointSize = 0.001f;

namespace anari {

namespace std_types {
using bvec4 = std::array<unsigned char, 4>;
} // namespace std_types
ANARI_TYPEFOR_SPECIALIZATION(std_types::bvec4, ANARI_UFIXED8_VEC4);
ANARI_TYPEFOR_SPECIALIZATION(vistle::Matrix4, ANARI_FLOAT32_MAT4);

template<typename T>
class ArrayIterator;

template<typename T>
ArrayIterator<T> mapParameterArray(Device device, Object object, const char *name, uint64_t numElem);

template<typename T>
class ArrayIterator {
    friend ArrayIterator<T> anari::mapParameterArray<>(Device device, Object object, const char *name,
                                                       uint64_t numElem);

public:
    explicit operator bool() const { return ptr != nullptr; }
    T &operator*() const { return *static_cast<T *>(ptr); }
    ArrayIterator &operator++()
    {
        ptr = (char *)ptr + stride;
        return *this;
    }
    ArrayIterator &operator+=(uint64_t n)
    {
        ptr = (char *)ptr + n * stride;
        return *this;
    }
    bool operator!=(const ArrayIterator &other) const { return ptr != other.ptr; }
    ArrayIterator operator+(uint64_t n)
    {
        ArrayIterator it = *this;
        it.ptr = (char *)ptr + n * stride;
        return it;
    }

    bool operator==(const ArrayIterator &rhs) { return ptr == rhs.ptr; }

private:
    void *ptr = nullptr;
    uint64_t stride = sizeof(T);
};

template<typename T>
ArrayIterator<T> mapParameterArray(Device device, Object object, const char *name, uint64_t numElem)
{
    ArrayIterator<T> it;
    it.ptr = anariMapParameterArray1D(device, object, name, ANARITypeFor<T>::value, numElem, &it.stride);
    if (it.stride == 0)
        it.stride = sizeof(T);
    return it;
}

void unmapParameterArray(Device device, Object object, const char *name)
{
    anariUnmapParameterArray(device, object, name);
}

} // namespace anari

template<typename T>
void ShmArrayDeleter(const void *userPtr, const void *appMemory)
{
    auto shm = reinterpret_cast<vistle::ShmVector<T> *>(const_cast<void *>(userPtr));
    shm->unref();
}

template<typename T>
inline anari::Array1D shareArray1D(anari::Device d, const vistle::ShmVector<T> &shm, uint64_t numItems1)
{
    assert(numItems1 <= shm->size());
    shm->ref();
    auto appMemory = shm->data();
    static auto deleter = &ShmArrayDeleter<T>;
    return anariNewArray1D(d, appMemory, deleter, &shm, anari::detail::getType<T>(), numItems1);
}

AnariRenderObject::AnariRenderObject(anari::Device device, int senderId, const std::string &senderPort,
                                     Object::const_ptr container, Object::const_ptr geometry, Object::const_ptr normals,
                                     Object::const_ptr texture)
: vistle::RenderObject(senderId, senderPort, container, geometry, normals, texture)
{
    updateBounds();
    auto t = geometry->getTransform();
    identityTransform = t.isIdentity();
    for (int i = 0; i < 16; ++i) {
        transform[i % 4][i / 4] = t(i / 4, i % 4);
    }
}

template<typename D, typename S>
struct ArrayAccess {
    ArrayAccess(const typename S::const_ptr &src);
    vistle::Index size() const;
    D operator[](vistle::Index idx) const;
};

template<typename D>
struct ArrayAccess<D, vistle::Vec<Scalar, 3>> {
    vistle::Index sz = vistle::InvalidIndex;
    const vistle::Scalar *x = nullptr, *y = nullptr, *z = nullptr;

    ArrayAccess(const vistle::Vec<Scalar, 3>::const_ptr &src)
    : sz(src->getSize()), x(&src->x()[0]), y(&src->y()[0]), z(&src->z()[0])
    {}
    vistle::Index size() const { return sz; }
    D operator[](vistle::Index i) const { return D{x[i], y[i], z[i]}; }
};

template<>
struct ArrayAccess<float, vistle::Vec<Scalar, 3>> {
    vistle::Index sz = vistle::InvalidIndex;
    const vistle::Scalar *x = nullptr, *y = nullptr, *z = nullptr;

    ArrayAccess(const vistle::Vec<Scalar, 3>::const_ptr &src)
    : sz(src->getSize()), x(&src->x()[0]), y(&src->y()[0]), z(&src->z()[0])
    {}
    vistle::Index size() const { return sz; }
    float operator[](vistle::Index i) const { return sqrtf(x[i] * x[i] + y[i] * y[i] + z[i] * z[i]); }
};

template<typename D>
struct ArrayAccess<D, vistle::CoordsWithRadius> {
    vistle::Index sz = vistle::InvalidIndex;
    const vistle::Scalar *r = nullptr;

    ArrayAccess(const vistle::CoordsWithRadius::const_ptr &src): sz(src->getSize()), r(&src->r()[0]) {}
    vistle::Index size() const { return sz; }
    D operator[](vistle::Index i) const { return D(r[i]); }
};

template<typename D, typename S>
struct ArrayAccess<D, vistle::Vec<S>> {
    vistle::Index sz = vistle::InvalidIndex;
    const S *x = nullptr;

    ArrayAccess(const typename vistle::Vec<S>::const_ptr &src): sz(src->getSize()), x(&src->x()[0]) {}
    vistle::Index size() const { return sz; }
    D operator[](vistle::Index i) const { return D(x[i]); }
};

template<typename T, typename Array>
bool copy(anari::Device device, anari::Geometry geom, const std::string &attr, const Array &array,
          const vistle::Index *cl = nullptr, vistle::Index N = 0)
{
    auto begin = anari::mapParameterArray<T>(device, geom, attr.c_str(), cl ? N : array.size());
    if (!begin)
        return false;

    auto end = begin + (cl ? N : array.size());
    Index idx = 0;
    for (auto it = begin; it != end; ++it) {
        *it = cl ? array[cl[idx]] : array[idx];
        ++idx;
    }

    anari::unmapParameterArray(device, geom, attr.c_str());
    return true;
}

void AnariRenderObject::create(anari::Device device)
{
    if (this->device == device) {
        return;
    }
    if (this->device) {
        anari::release(this->device, surface);
        surface = nullptr;
    }
    this->device = device;
    if (!device) {
        return;
    }

    if (!geometry || geometry->isEmpty()) {
        return;
    }

    //std::cerr << "Geometry: " << *geometry << std::endl;

    anari::Geometry geom = nullptr;
    bool flattenCoordinates = false;
    bool useNormals = false;
    const vistle::Index *cl = nullptr;
    vistle::Index numCorners = 0;
    if (auto tri = Triangles::as(geometry)) {
        geom = anari::newObject<anari::Geometry>(device, "triangle");
        useNormals = true;
        if (tri->getNumCorners() > 0) {
            if (auto begin = anari::mapParameterArray<anari::std_types::uvec3>(device, geom, "primitive.index",
                                                                               tri->getNumCorners() / 3)) {
                auto end = begin + tri->getNumCorners() / 3;
                const auto *cl = &tri->cl()[0];
                for (auto it = begin; it != end; ++it) {
                    *it = {unsigned(*cl++), unsigned(*cl++), unsigned(*cl++)};
                }
                anari::unmapParameterArray(device, geom, "primitive.index");
            }
        }
    } else if (auto quad = Quads::as(geometry)) {
        geom = anari::newObject<anari::Geometry>(device, "quad");
        useNormals = true;
        if (quad->getNumCorners() > 0) {
            if (auto begin = anari::mapParameterArray<anari::std_types::uvec4>(device, geom, "primitive.index",
                                                                               quad->getNumCorners() / 4)) {
                auto end = begin + quad->getNumCorners() / 4;
                const auto *cl = &quad->cl()[0];
                for (auto it = begin; it != end; ++it) {
                    *it = {unsigned(*cl++), unsigned(*cl++), unsigned(*cl++), unsigned(*cl++)};
                }
                anari::unmapParameterArray(device, geom, "primitive.index");
            }
        }
    } else if (auto poly = Polygons::as(geometry)) {
        Index ntri = poly->getNumCorners() - 2 * poly->getNumElements();
        assert(ntri >= 0);

        geom = anari::newObject<anari::Geometry>(device, "triangle");
        useNormals =
            normals && normals->guessMapping(poly) == vistle::DataBase::Vertex; // FIXME: remap per-primitive normals
        if (auto begin = anari::mapParameterArray<anari::std_types::uvec3>(device, geom, "primitive.index", ntri)) {
            const auto *cl = &poly->cl()[0];
            const auto *el = &poly->el()[0];

            auto it = begin;
            for (Index i = 0; i < poly->getNumElements(); ++i) {
                const Index start = el[i];
                const Index end = el[i + 1];
                const Index nvert = end - start;
                const Index last = end - 1;
                for (Index v = 0; v < nvert - 2; ++v) {
                    const Index v2 = v / 2;
                    if (v % 2) {
                        *it = {unsigned(cl[last - v2]), unsigned(cl[start + v2 + 1]), unsigned(cl[last - v2 - 1])};
                    } else {
                        *it = {unsigned(cl[start + v2]), unsigned(cl[start + v2 + 1]), unsigned(cl[last - v2])};
                    }
                    ++it;
                }
            }
            assert(it == begin + ntri);

            anari::unmapParameterArray(device, geom, "primitive.index");
        }
    } else if (auto sph = Spheres::as(geometry)) {
        geom = anari::newObject<anari::Geometry>(device, "sphere");
        ArrayAccess<float, vistle::CoordsWithRadius> rad(sph);
        copy<float>(device, geom, "vertex.radius", rad);
    } else if (auto point = Points::as(geometry)) {
        geom = anari::newObject<anari::Geometry>(device, "sphere");
        anari::setParameter(device, geom, "radius", pointSize);
    } else if (auto tube = Tubes::as(geometry)) {
        Index nStrips = tube->getNumTubes();
        Index nPoints = tube->getNumCoords();
        geom = anari::newObject<anari::Geometry>(device, "curve");

        // FIXME: handle this
        // const Tubes::CapStyle startStyle = tube->startStyle(), jointStyle = tube->jointStyle(), endStyle = tube->endStyle();

        if (auto begin = anari::mapParameterArray<Index>(device, geom, "primitive.index", nPoints - nStrips)) {
            const auto *el = &tube->components()[0];
            auto it = begin;
            for (Index e = 0; e < nStrips; ++e) {
                for (Index idx = el[e]; idx < el[e + 1] - 1; ++idx) {
                    *it = idx;
                    ++it;
                }
            }
            anari::unmapParameterArray(device, geom, "primitive.index");
        }

        ArrayAccess<float, vistle::CoordsWithRadius> rad(tube);
        copy<float>(device, geom, "vertex.radius", rad);
    } else if (auto line = Lines::as(geometry)) {
        Index nStrips = line->getNumElements();
        Index nPoints = line->getNumCorners();
        std::cerr << "Lines: #strips: " << nStrips << ", #corners: " << nPoints << std::endl;
        geom = anari::newObject<anari::Geometry>(device, "curve");

        if (nPoints > 0) {
            flattenCoordinates = true;
            cl = &line->cl()[0];
            numCorners = nPoints;
        }

        if (auto begin = anari::mapParameterArray<Index>(device, geom, "primitive.index", nPoints - nStrips)) {
            const auto *el = &line->el()[0];
            auto it = begin;
            for (Index e = 0; e < nStrips; ++e) {
                for (Index idx = el[e]; idx < el[e + 1] - 1; ++idx) {
                    *it = idx;
                    ++it;
                }
            }
            anari::unmapParameterArray(device, geom, "primitive.index");
        }

        anari::setParameter(device, geom, "radius", pointSize);
    }

    if (!geom)
        return;

    if (auto coords = Coords::as(geometry)) {
        ArrayAccess<anari::std_types::vec3, vistle::Vec<Scalar, 3>> crd(coords);
        if (flattenCoordinates)
            copy<anari::std_types::vec3>(device, geom, "vertex.position", crd, cl, numCorners);
        else
            copy<anari::std_types::vec3>(device, geom, "vertex.position", crd);
    }

    if (normals && useNormals) {
        std::string attr = "vertex";
        if (normals->guessMapping(geometry) == DataBase::Element)
            attr = "primitive";
        attr += ".normal";
        ArrayAccess<anari::std_types::vec3, vistle::Vec<Scalar, 3>> norm(normals);
        copy<anari::std_types::vec3>(device, geom, attr, norm, cl, numCorners);
    }

    bool haveColor = false;
    bool useSampler = false;
    std::string attr = "vertex";
    if (this->mapdata) {
        if (this->mapdata->guessMapping(geometry) == DataBase::Element)
            attr = "primitive";
    }
    if (auto t = Texture1D::as(this->mapdata)) {
        haveColor = true;
        attr += ".color";
        if (auto begin = anari::mapParameterArray<anari::std_types::bvec4>(device, geom, attr.c_str(), t->getSize())) {
            auto end = begin + t->getSize();
            const auto *ct = &t->pixels()[0];
            auto w = t->getWidth();
            const auto *tc = &t->coords()[0];
            for (auto it = begin; it != end; ++it) {
                Index idx = std::min(Index(*tc++ * w), w - 1);
                *it = {ct[idx * 4], ct[idx * 4 + 1], ct[idx * 4 + 2], ct[idx * 4 + 3]};
            }
            anari::unmapParameterArray(device, geom, attr.c_str());
        }
    } else if (auto s = Vec<Scalar, 1>::as(this->mapdata)) {
        useSampler = true;
        attr += ".attribute0";
        if (cl) {
            ArrayAccess<float, vistle::Vec<Scalar>> scal(s);
            copy<float>(device, geom, attr, scal, cl, numCorners);
        } else {
            auto arr = shareArray1D(device, s->d()->x[0], s->getSize());
            anari::setAndReleaseParameter(device, geom, attr.c_str(), arr);
        }
    } else if (auto vec = Vec<Scalar, 3>::as(this->mapdata)) {
        useSampler = true;
        attr += ".attribute0";
        ArrayAccess<float, vistle::Vec<Scalar, 3>> vect(vec);
        copy<float>(device, geom, attr, vect, cl, numCorners);
    } else if (auto iscal = Vec<Index>::as(this->mapdata)) {
        useSampler = true;
        attr += ".attribute0";
        ArrayAccess<float, vistle::Vec<Index>> index(iscal);
        copy<float>(device, geom, attr, index, cl, numCorners);
    }

    std::string name = container->getName();
    anari::setParameter(device, geom, "name", "geo:" + name);
    anari::commitParameters(device, geom);

    surface = anari::newObject<anari::Surface>(device);
    anari::setParameter(device, surface, "name", "surf:" + name);
    anari::setAndReleaseParameter(device, surface, "geometry", geom);

    auto mat = anari::newObject<anari::Material>(device, "matte");
    anari::setParameter(device, mat, "name", "mat:" + name);
    if (haveColor) {
        anari::setParameter(device, mat, "color", "color");
    } else if (useSampler) {
        if (colorMap)
            anari::setParameter(device, mat, "color", colorMap->sampler);
        else
            anari::unsetParameter(device, mat, "color");
    } else if (hasSolidColor) {
        anari::setParameter(device, mat, "color",
                            anari::std_types::vec4{solidColor[0], solidColor[1], solidColor[2], solidColor[3]});
    }
    anari::commitParameters(device, mat);
    anari::setAndReleaseParameter(device, surface, "material", mat);
    anari::commitParameters(device, surface);
}

AnariRenderObject::~AnariRenderObject()
{
    if (device) {
        anari::release(device, surface);
        surface = nullptr;
    }
}

void AnariColorMap::create(anari::Device dev)
{
    if (device != dev) {
        if (device)
            anari::release(device, sampler);
        sampler = nullptr;
        device = dev;
        if (dev) {
            sampler = anari::newObject<anari::Sampler>(device, "image1D");
        }
    }
    if (device) {
        assert(sampler);
        anari::setParameter(device, sampler, "name", "color:" + species);
        anari::setParameter(device, sampler, "inAttribute", "attribute0");
        anari::setParameter(device, sampler, "filter", "nearest");
        if (tex) {
            if (auto begin =
                    anari::mapParameterArray<anari::std_types::bvec4>(device, sampler, "image", tex->getWidth())) {
                auto end = begin + tex->getWidth();
                const auto *ct = &tex->pixels()[0];
                for (auto it = begin; it != end; ++it) {
                    *it = {ct[0], ct[1], ct[2], ct[3]};
                    ct += 4;
                }
                anari::unmapParameterArray(device, sampler, "image");
            }
            auto range = tex->getMax() - tex->getMin();
            if (range > 0.f) {
                anari::std_types::mat4 smat{
                    {{1.f / range, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}}};
                anari::setParameter(device, sampler, "inTransform", smat);
                anari::setParameter(device, sampler, "inOffset",
                                    anari::std_types::vec4{-tex->getMin() / range, 0.f, 0.f, 0.f});
            }
        } else {
            if (auto begin = anari::mapParameterArray<anari::std_types::bvec4>(device, sampler, "image", 2)) {
                auto it = begin;
                *it = {63, 63, 63, 255};
                ++it;
                *it = {191, 191, 191, 255};
                ++it;
                assert(it == begin + 2);
                anari::unmapParameterArray(device, sampler, "image");
            }
        }
        anari::commitParameters(device, sampler);
    }
}

AnariColorMap::~AnariColorMap()
{
    if (device) {
        anari::release(device, sampler);
    }
}
