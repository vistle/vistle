#ifndef RAY_RENDEROBJECT_H
#define RAY_RENDEROBJECT_H

#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <core/vector.h>
#include <core/object.h>
#include <core/normals.h>
#include <core/texture1d.h>

#include <renderer/renderobject.h>

static const int MaxPacketSize = 8;

static const RTCAlgorithmFlags intersections = RTC_INTERSECT1|RTC_INTERSECT4|RTC_INTERSECT8;
static const RTCSceneFlags sceneFlags = RTC_SCENE_COHERENT;

static const unsigned int RayEnabled = 0xffffffff;
static const unsigned int RayDisabled = 0;

struct RenderObjectData {

   struct Vertex   { float x,y,z,align; };
   struct Triangle { unsigned int v0, v1, v2; };

   int t;
   RTCScene scene;
   int geomId;
   unsigned instId;
   Triangle *indexBuffer;

   unsigned int texWidth;
   unsigned char *texData;
   vistle::Scalar *texCoords;
};

struct RayRenderObject: public vistle::RenderObject, RenderObjectData {

   RayRenderObject(int senderId, const std::string &senderPort,
         vistle::Object::const_ptr container,
         vistle::Object::const_ptr geometry,
         vistle::Object::const_ptr normals,
         vistle::Object::const_ptr colors,
         vistle::Object::const_ptr texture);

   ~RayRenderObject();
};
#endif
