#ifndef RAY_RENDEROBJECT_H
#define RAY_RENDEROBJECT_H

#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <core/vector.h>
#include <core/object.h>
#include <core/normals.h>
#include <core/texture1d.h>

static const int MaxPacketSize = 8;

static const RTCAlgorithmFlags intersections = RTC_INTERSECT1|RTC_INTERSECT4|RTC_INTERSECT8;
static const RTCSceneFlags sceneFlags = RTC_SCENE_COHERENT;

static const unsigned int RayEnabled = 0xffffffff;
static const unsigned int RayDisabled = 0;

struct RenderObject {

   struct Vertex   { float x,y,z,align; };
   struct Triangle { unsigned int v0, v1, v2; };

   RenderObject(vistle::Object::const_ptr container,
   vistle::Object::const_ptr geometry,
   vistle::Object::const_ptr colors,
   vistle::Object::const_ptr normals,
   vistle::Object::const_ptr texture);

   ~RenderObject();

   int t;
   vistle::Object::const_ptr container;
   vistle::Object::const_ptr geometry;
   vistle::Object::const_ptr colors;
   vistle::Normals::const_ptr normals;
   vistle::Texture1D::const_ptr texture;

   RTCScene scene;
   unsigned geomId;
   unsigned instId;
   vistle::Vector bMin, bMax;
   std::vector<unsigned> elementMap;
   Triangle *indexBuffer;
};
#endif
