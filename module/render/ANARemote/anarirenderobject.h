#ifndef VISTLE_ANAREMOTE_ANARIRENDEROBJECT_H
#define VISTLE_ANAREMOTE_ANARIRENDEROBJECT_H

#include <vector>
#include <memory>

#include <vistle/core/vector.h>
#include <vistle/core/object.h>
#include <vistle/core/normals.h>

#include <vistle/renderer/renderobject.h>

#include <anari/anari_cpp/ext/std.h>
#include <anari/anari_cpp.hpp>
namespace anari {
namespace std_types {
using bvec4 = std::array<unsigned char, 4>;
} // namespace std_types

} // namespace anari

struct AnariColorMap {
    ~AnariColorMap();

    void create(anari::Device device);

    std::string species;
    double min = 0.;
    double max = 1.;
    bool blendWithMaterial = false;
    using RGBA = anari::std_types::bvec4;
    std::vector<RGBA> rgba;
    anari::Device device = nullptr;
    anari::Sampler sampler = nullptr;
};

struct AnariRenderObject: public vistle::RenderObject {
    static float pointSize;

    AnariRenderObject(anari::Device device, int senderId, const std::string &senderPort,
                      vistle::Object::const_ptr container, vistle::Object::const_ptr geometry,
                      vistle::Object::const_ptr normals, vistle::Object::const_ptr texture);

    ~AnariRenderObject();
    void create(anari::Device device);

    anari::Device device = nullptr;
    anari::Surface surface = nullptr;
    AnariColorMap *colorMap = nullptr;

    using mat4 = anari::std_types::mat4;
    mat4 transform;
    bool identityTransform = false;
};

#endif
