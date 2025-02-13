#ifndef VISTLE_DISCOVERAY_RAYRENDEROBJECT_H
#define VISTLE_DISCOVERAY_RAYRENDEROBJECT_H

#include <vector>
#include <memory>

#include <vistle/core/vector.h>
#include <vistle/core/object.h>
#include <vistle/core/normals.h>
#include <vistle/core/texture1d.h>

#include <vistle/renderer/renderobject.h>

#include "renderobjectdata_ispc.h"
#include "render_ispc.h"

#include "common.h"

struct RayColorMap {
    void deinit();

    vistle::Texture1D::const_ptr tex;
    std::shared_ptr<ispc::ColorMapData> cmap;
};

struct RayRenderObject: public vistle::RenderObject {
    static float pointSize;

    RayRenderObject(RTCDevice device, int senderId, const std::string &senderPort, vistle::Object::const_ptr container,
                    vistle::Object::const_ptr geometry, vistle::Object::const_ptr normals,
                    vistle::Object::const_ptr texture);

    ~RayRenderObject();

    std::unique_ptr<ispc::RenderObjectData> data;
    std::unique_ptr<ispc::ColorMapData> cmap;
    std::vector<vistle::Scalar> tcoord;
};
#endif
