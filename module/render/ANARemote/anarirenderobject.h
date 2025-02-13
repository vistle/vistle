#ifndef VISTLE_ANAREMOTE_ANARIRENDEROBJECT_H
#define VISTLE_ANAREMOTE_ANARIRENDEROBJECT_H

#include <vector>
#include <memory>

#include <vistle/core/vector.h>
#include <vistle/core/object.h>
#include <vistle/core/normals.h>
#include <vistle/core/texture1d.h>

#include <vistle/renderer/renderobject.h>

#include <anari/anari_cpp/ext/std.h>
#include <anari/anari_cpp.hpp>

struct AnariColorMap {
    ~AnariColorMap();

    void create(anari::Device device);

    std::string species;
    vistle::Texture1D::const_ptr tex;
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
