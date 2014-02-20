#include <core/polygons.h>
#include <core/triangles.h>
#include <core/texture1d.h>

#include <core/assert.h>

#include "renderobject.h"

using namespace vistle;

RenderObject::RenderObject(Object::const_ptr container,
      Object::const_ptr geometry,
      Object::const_ptr colors,
      Object::const_ptr normals,
      Object::const_ptr texture)
: t(-1)
, container(container)
, geometry(geometry)
, colors(colors)
, normals(normals)
, texture(texture)
, geomId(RTC_INVALID_GEOMETRY_ID)
, instId(RTC_INVALID_GEOMETRY_ID)
, indexBuffer(nullptr)
{
   const Scalar smax = std::numeric_limits<Scalar>::max();
   bMin = Vector(smax, smax, smax);
   bMax = Vector(-smax, -smax, -smax);

   scene = rtcNewScene(RTC_SCENE_STATIC|sceneFlags, intersections);

   if (auto tri = Triangles::as(geometry)) {

      geomId = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, tri->getNumElements(), tri->getNumCoords());
      std::cerr << "Tri: #tri: " << tri->getNumElements() << ", #coord: " << tri->getNumCoords() << std::endl;

      Vertex* vertices = (Vertex*) rtcMapBuffer(scene,geomId,RTC_VERTEX_BUFFER);
      for (Index i=0; i<tri->getNumCoords(); ++i) {
         vertices[i].x = tri->x()[i];
         vertices[i].y = tri->y()[i];
         vertices[i].z = tri->z()[i];

         if (vertices[i].x < bMin[0])
            bMin[0] = vertices[i].x;
         if (vertices[i].y < bMin[1])
            bMin[1] = vertices[i].y;
         if (vertices[i].z < bMin[2])
            bMin[2] = vertices[i].z;
         if (vertices[i].x > bMax[0])
            bMax[0] = vertices[i].x;
         if (vertices[i].y > bMax[1])
            bMax[1] = vertices[i].y;
         if (vertices[i].z > bMax[2])
            bMax[2] = vertices[i].z;
      }
      rtcUnmapBuffer(scene,geomId,RTC_VERTEX_BUFFER);

      indexBuffer = new Triangle[tri->getNumElements()];
      rtcSetBuffer(scene, geomId, RTC_INDEX_BUFFER, indexBuffer, 0, sizeof(Triangle));
      Triangle* triangles = (Triangle*) rtcMapBuffer(scene,geomId,RTC_INDEX_BUFFER);
      for (Index i=0; i<tri->getNumElements(); ++i) {
         triangles[i].v0 = tri->cl()[i*3];
         triangles[i].v1 = tri->cl()[i*3+1];
         triangles[i].v2 = tri->cl()[i*3+2];
      }
      rtcUnmapBuffer(scene,geomId,RTC_INDEX_BUFFER);
   } else if (auto poly = Polygons::as(geometry)) {

      Index ntri = poly->getNumCorners()-2*poly->getNumElements();
      vassert(ntri >= 0);

      geomId = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, ntri, poly->getNumCoords());
      std::cerr << "Poly: #tri: " << poly->getNumCorners()-2*poly->getNumElements() << ", #coord: " << poly->getNumCoords() << std::endl;

      Vertex* vertices = (Vertex*) rtcMapBuffer(scene,geomId,RTC_VERTEX_BUFFER);
      for (Index i=0; i<poly->getNumCoords(); ++i) {
         vertices[i].x = poly->x()[i];
         vertices[i].y = poly->y()[i];
         vertices[i].z = poly->z()[i];

         if (vertices[i].x < bMin[0])
            bMin[0] = vertices[i].x;
         if (vertices[i].y < bMin[1])
            bMin[1] = vertices[i].y;
         if (vertices[i].z < bMin[2])
            bMin[2] = vertices[i].z;
         if (vertices[i].x > bMax[0])
            bMax[0] = vertices[i].x;
         if (vertices[i].y > bMax[1])
            bMax[1] = vertices[i].y;
         if (vertices[i].z > bMax[2])
            bMax[2] = vertices[i].z;
      }
      rtcUnmapBuffer(scene,geomId,RTC_VERTEX_BUFFER);

      indexBuffer = new Triangle[ntri];
      rtcSetBuffer(scene, geomId, RTC_INDEX_BUFFER, indexBuffer, 0, sizeof(Triangle));
      Triangle* triangles = (Triangle*) rtcMapBuffer(scene,geomId,RTC_INDEX_BUFFER);
      Index t = 0;
      for (Index i=0; i<poly->getNumElements(); ++i) {
         const Index start = poly->el()[i];
         const Index nvert = poly->el()[i+1]-start;
         const Index last = start+nvert-1;
         for (Index v=0; v<nvert-2; ++v) {
            const int v2 = v/2;
            if (v%2) {
               triangles[t].v0 = poly->cl()[start+v2];
               triangles[t].v1 = poly->cl()[last-v2];
               triangles[t].v2 = poly->cl()[start+v2+1];
            } else {
               triangles[t].v0 = poly->cl()[last-v2];
               triangles[t].v1 = poly->cl()[start+v2+1];
               triangles[t].v2 = poly->cl()[last-v2-1];
            }
            ++t;
         }
      }
      vassert(t == ntri);
      rtcUnmapBuffer(scene,geomId,RTC_INDEX_BUFFER);
   }

   rtcCommit(scene);
}

RenderObject::~RenderObject() {

   rtcDeleteScene(scene);
   delete indexBuffer;
}
