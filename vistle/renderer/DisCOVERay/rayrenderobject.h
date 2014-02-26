#ifndef RAY_RENDEROBJECT_H
#define RAY_RENDEROBJECT_H

#include <vector>
#include <memory>

#include <core/vector.h>
#include <core/object.h>
#include <core/normals.h>
#include <core/texture1d.h>

#include <renderer/renderobject.h>

#include "render.h"

struct RayRenderObject: public vistle::RenderObject {

   RayRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture);

   ~RayRenderObject();

   std::unique_ptr<RenderObjectData> data;
};
#endif
