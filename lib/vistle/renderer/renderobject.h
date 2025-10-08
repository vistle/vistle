#ifndef VISTLE_RENDERER_RENDEROBJECT_H
#define VISTLE_RENDERER_RENDEROBJECT_H

#include <vector>

#include <vistle/util/enum.h>
#include <vistle/core/vector.h>
#include <vistle/core/object.h>
#include <vistle/core/normals.h>

#include "export.h"

namespace vistle {

class V_RENDEREREXPORT RenderObject {
public:
    RenderObject(int senderId, const std::string &senderPort, vistle::Object::const_ptr container,
                 vistle::Object::const_ptr geometry, vistle::Object::const_ptr normals,
                 vistle::Object::const_ptr mapdata);

    virtual ~RenderObject();

    void updateBounds();
    void computeBounds();
    bool boundsValid() const;

    int creatorId;
    int senderId;
    std::string senderPort;
    std::string variant;
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(InitialVariantVisibility, (DontChange)(Hidden)(Visible));
    InitialVariantVisibility visibility = DontChange;

    vistle::Object::const_ptr container;
    vistle::Object::const_ptr geometry;
    vistle::Normals::const_ptr normals;
    vistle::DataBase::const_ptr mapdata;

    int timestep = -1;
    bool bComputed = false;
    bool bValid = false;
    vistle::Vector3 bMin, bMax;

    bool hasSolidColor = false;
    vistle::Vector4 solidColor;
};

} // namespace vistle
#endif
