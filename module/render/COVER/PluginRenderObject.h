#ifndef VISTLE_COVER_PLUGINRENDEROBJECT_H
#define VISTLE_COVER_PLUGINRENDEROBJECT_H

#include <memory>
#include <string>

#include <vistle/core/object.h>
#include <vistle/renderer/renderobject.h>
#include <VistlePluginUtil/VistleRenderObject.h>

class PluginRenderObject: public vistle::RenderObject {
public:
    PluginRenderObject(int senderId, const std::string &senderPort, vistle::Object::const_ptr container,
                       vistle::Object::const_ptr geometry, vistle::Object::const_ptr normals,
                       vistle::Object::const_ptr texture);

    ~PluginRenderObject() override;

    std::shared_ptr<VistleRenderObject> coverRenderObject;
};

#endif
