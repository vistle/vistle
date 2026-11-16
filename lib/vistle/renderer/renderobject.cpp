#include <boost/algorithm/string/predicate.hpp>
#include <vistle/core/coords.h>
#include <vistle/core/points.h>
#include <vistle/core/lines.h>
#include <vistle/core/vec.h>
#include <vistle/config/access.h>
#include <vistle/config/value.h>
#include <cassert>
#include <tuple>
#include <utility>

#include "renderobject.h"

namespace vistle {

namespace {
std::mutex rgb_txt_mutex;
bool rgb_initialized = false;
std::map<std::string, Vector4> rgb_name;

std::string rgb_normalize_name(std::string name)
{
    const char *p = name.c_str();
    std::string colorname;
    while (*p) {
        auto c = static_cast<unsigned char>(*p);
        if (!isspace(c))
            colorname.push_back(tolower(c));
        ++p;
    }
    return colorname;
}

bool rgb_txt_init_from_file()
{
    std::lock_guard<std::mutex> guard(rgb_txt_mutex);
    if (rgb_initialized)
        return rgb_name.size() > 1;

    rgb_initialized = true;
    auto config = config::Access();
    double min = *config.value<double>("renderer", "rgb", "min", 0.0);
    double max = *config.value<double>("renderer", "rgb", "max", 1.0);
    std::string rgbfile = *config.value<std::string>("renderer", "rgb", "file", "");

    for (std::string prefix: {"", "/usr", "/usr/local", "/usr/X11R6", "/opt/X11", "/opt/homebrew"}) {
        std::string filename;
        if (prefix.empty()) {
            if (!rgbfile.empty()) {
                filename = rgbfile;
            } else if (const char *cd = getenv("COVISEDIR")) {
                filename = cd;
                filename += "/share/covise/rgb.txt";
            } else {
                continue;
            }
        } else {
            filename = prefix + "/share/X11/rgb.txt";
        }

        if (FILE *fp = fopen(filename.c_str(), "r")) {
            char line[500];
            while (fgets(line, sizeof(line), fp)) {
                if (line[0] == '!')
                    continue;
                int r = 255, g = 255, b = 255;
                char name[150];
                sscanf(line, "%d%d%d %100[a-zA-z0-9 ]", &r, &g, &b, name);
                std::string colorname = rgb_normalize_name(name);
                rgb_name[colorname] = Vector4(r / 255.f, g / 255.f, b / 255.f, 1.f);

                for (int i = 0; i < 3; ++i) {
                    rgb_name[colorname][i] = rgb_name[colorname][i] * (max - min) + min;
                }
                //std::cerr << "color " << colorname << " -> " << rgb_name[colorname] << std::endl;
            }
            fclose(fp);
            break;
        }
    }

    return rgb_name.size() > 1;
}

bool rgb_color(const std::string color, Vector4 &rgb)
{
    if (!rgb_txt_init_from_file())
        return false;

    std::string colorname = rgb_normalize_name(color);
    auto it = rgb_name.find(colorname);
    if (it == rgb_name.end())
        return false;

    rgb = it->second;
    return true;
}

} // namespace

bool ColorMapKey::operator<(const ColorMapKey &o) const
{
#ifdef COLORS_BY_SOURCE
    if (sourceModule != o.sourceModule) {
        return sourceModule < o.sourceModule;
    }
#endif
    return species < o.species;
}

RenderObject::RenderObject(int senderId, const std::string &senderPort, Object::const_ptr container,
                           Object::const_ptr geometry, Object::const_ptr normals, Object::const_ptr mapdata)
: senderId(senderId)
, senderPort(senderPort)
, container(container)
, geometry(geometry)
, normals(Normals::as(normals))
, mapdata(DataBase::as(mapdata))
, timestep(-1)
, hasSolidColor(false)
, solidColor(0., 0., 0., 0.)
{
    std::string color;
    if (geometry && geometry->hasAttribute(attribute::Color)) {
        color = geometry->getAttribute(attribute::Color);
        std::cerr << "RenderObject color: " << color << " from geometry " << geometry->getName() << std::endl;
    } else if (container && container->hasAttribute(attribute::Color)) {
        color = container->getAttribute(attribute::Color);
        std::cerr << "RenderObject color: " << color << " from container " << container->getName() << std::endl;
    }

    if (!color.empty()) {
        std::stringstream str(color);
        char ch;
        str >> ch;
        if (ch == '#') {
            str << std::hex;
            unsigned long c = 0xffffffffL;
            str >> c;
            float a = 1.f;
            if (color.length() > 7) {
                a = (c & 0xffL) / 255.f;
                c >>= 8;
            }
            float b = (c & 0xffL) / 255.f;
            c >>= 8;
            float g = (c & 0xffL) / 255.f;
            c >>= 8;
            float r = (c & 0xffL) / 255.f;
            c >>= 8;

            hasSolidColor = true;
            solidColor = Vector4(r, g, b, a);
        } else {
            if (rgb_color(color, solidColor)) {
                hasSolidColor = true;
            }
        }
    }
    if (timestep < 0 && mapdata) {
        timestep = mapdata->getTimestep();
    }
    if (timestep < 0 && geometry) {
        timestep = geometry->getTimestep();
    }
    if (timestep < 0 && normals) {
        timestep = normals->getTimestep();
    }
    if (timestep < 0 && container) {
        timestep = container->getTimestep();
    }

    if (container)
        variant = container->getAttribute(attribute::Variant);
    if (geometry && variant.empty())
        variant = geometry->getAttribute(attribute::Variant);

    if (boost::algorithm::ends_with(variant, "_on")) {
        variant = variant.substr(0, variant.length() - 3);
        visibility = Visible;
    } else if (boost::algorithm::ends_with(variant, "_off")) {
        variant = variant.substr(0, variant.length() - 4);
        visibility = Hidden;
    }
}

RenderObject::~RenderObject()
{}

void RenderObject::updateBounds()
{
    if (bComputed)
        return;
    computeBounds();
}

namespace {

std::pair<Vector3, Vector3> computeTransformedUnitCubeBounds(const Matrix4 &T, bool identity)
{
    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 rMin(smax, smax, smax), rMax(-smax, -smax, -smax);
    for (int i = 0; i < 8; ++i) {
        Vector3 v(0, 0, 0);
        if (i % 2) {
            v += Vector3(1, 0, 0);
        } else {
            v -= Vector3(1, 0, 0);
        }
        if ((i / 2) % 2) {
            v += Vector3(0, 1, 0);
        } else {
            v -= Vector3(0, 1, 0);
        }
        if ((i / 4) % 2) {
            v += Vector3(0, 0, 1);
        } else {
            v -= Vector3(0, 0, 1);
        }
        if (!identity)
            v = transformPoint(T, v);
        for (int c = 0; c < 3; ++c) {
            rMin[c] = std::min(rMin[c], v[c]);
            rMax[c] = std::max(rMax[c], v[c]);
        }
    }
    return {rMin, rMax};
}

std::pair<Vector3, Vector3> computeBoundsForElementBasedRadii(Indexed::const_ptr indexed, Coords::const_ptr coords,
                                                              const Scalar *radii, const Vector3 &rMin,
                                                              const Vector3 &rMax)
{
    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 bMin(smax, smax, smax), bMax(-smax, -smax, -smax);

    Matrix4 T = indexed->getTransform();
    bool identity = T.isIdentity();

    auto el = indexed->el().data();
    auto cl = indexed->cl().data();
    const Scalar *x = coords->x(0), *y = coords->x(1), *z = coords->x(2);
    for (Index i = 0; i < indexed->getNumElements(); ++i) {
        Scalar r = radii[i];

        for (Index j = el[i]; j < el[i + 1]; ++j) {
            // if cl is empty, it implicitly becomes [0,1,2,...]), so in that case
            // we can simply use j as the index into the coords array
            Index coordIdx = (indexed->cl().size() > 0) ? cl[j] : j;
            Vector3 p(x[coordIdx], y[coordIdx], z[coordIdx]);
            if (!identity)
                p = transformPoint(T, p);
            for (int c = 0; c < 3; ++c) {
                bMin[c] = std::min(bMin[c], p[c] + r * rMin[c]);
                bMax[c] = std::max(bMax[c], p[c] + r * rMax[c]);
            }
        }
    }
    return {bMin, bMax};
}

std::pair<Vector3, Vector3> computeBoundsForVertexBasedRadii(Object::const_ptr geometry, Coords::const_ptr coords,
                                                             const Scalar *radii, const Vector3 &rMin,
                                                             const Vector3 &rMax)
{
    const Scalar smax = std::numeric_limits<Scalar>::max();
    Vector3 bMin(smax, smax, smax), bMax(-smax, -smax, -smax);

    Matrix4 T = geometry->getTransform();
    bool identity = T.isIdentity();

    const auto nc = coords->getNumCoords();
    const Scalar *x = coords->x(0), *y = coords->x(1), *z = coords->x(2);

    for (Index i = 0; i < nc; ++i) {
        Scalar r = radii[i];
        Vector3 p(x[i], y[i], z[i]);
        if (!identity)
            p = transformPoint(T, p);
        for (int c = 0; c < 3; ++c) {
            bMin[c] = std::min(bMin[c], p[c] + r * rMin[c]);
            bMax[c] = std::max(bMax[c], p[c] + r * rMax[c]);
        }
    }
    return {bMin, bMax};
}

} // namespace

void RenderObject::computeBounds()
{
    if (!geometry)
        return;

    const Scalar smax = std::numeric_limits<Scalar>::max();
    bMin = Vector3(smax, smax, smax);
    bMax = Vector3(-smax, -smax, -smax);

    Matrix4 T = geometry->getTransform();
    bool identity = T.isIdentity();
    if (auto coords = Coords::as(geometry)) {
        Vec<Scalar>::const_ptr radiiPtr;
        if (auto lines = Lines::as(geometry)) {
            if (lines->radius()) {
                radiiPtr = lines->radius();
            }
        } else if (auto points = Points::as(geometry)) {
            if (points->radius()) {
                radiiPtr = points->radius();
            }
        }

        if (auto radii = radiiPtr->x().data()) {
            Vector3 rMin, rMax;
            std::tie(rMin, rMax) = computeTransformedUnitCubeBounds(T, identity);
            if (auto lines = Lines::as(geometry); lines && radiiPtr->guessMapping(geometry) == DataBase::Element) {
                assert(radiiPtr->getSize() == lines->getNumElements());
                std::tie(bMin, bMax) = computeBoundsForElementBasedRadii(lines, coords, radii, rMin, rMax);
            } else {
                assert(radiiPtr->getSize() == coords->getNumCoords());
                std::tie(bMin, bMax) = computeBoundsForVertexBasedRadii(geometry, coords, radii, rMin, rMax);
            }

        } else {
            if (identity) {
                auto b = coords->getBounds();
                bMin = b.first;
                bMax = b.second;
            } else {
                const auto nc = coords->getNumCoords();
                const Scalar *x = coords->x(0), *y = coords->x(1), *z = coords->x(2);
                for (Index i = 0; i < nc; ++i) {
                    Vector3 p(x[i], y[i], z[i]);
                    if (!identity)
                        p = transformPoint(T, p);
                    for (int c = 0; c < 3; ++c) {
                        if (p[c] < bMin[c])
                            bMin[c] = p[c];
                        if (p[c] > bMax[c])
                            bMax[c] = p[c];
                    }
                }
            }
        }
    }

    bValid = true;
    for (int c = 0; c < 3; ++c) {
        if (bMin[c] > bMax[c])
            bValid = false;
    }

    bComputed = true;
}

bool RenderObject::boundsValid() const
{
    return bValid;
}

} // namespace vistle
