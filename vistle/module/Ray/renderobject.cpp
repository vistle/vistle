#include <core/polygons.h>
#include <core/triangles.h>

#include <core/assert.h>

#include "renderobject.h"

using namespace vistle;

RenderObject::RenderObject(int senderId, const std::string &senderPort,
      Object::const_ptr container,
      Object::const_ptr geometry,
      Object::const_ptr colors,
      Object::const_ptr normals,
      Object::const_ptr texture)
: senderId(senderId)
, senderPort(senderPort)
, container(container)
, geometry(geometry)
, colors(colors)
, normals(Normals::as(normals))
, texture(Texture1D::as(texture))
, hasSolidColor(false)
, solidColor(0., 0., 0., 0.)
{
   t = -1;
   geomId = RTC_INVALID_GEOMETRY_ID;
   instId = RTC_INVALID_GEOMETRY_ID;
   indexBuffer = nullptr;
   texWidth = 0;
   texData = nullptr;
   texCoords = nullptr;
   if (this->texture) {
      texWidth = this->texture->getWidth();
      texData = this->texture->pixels().data();
      texCoords = this->texture->coords().data();
   }

   const Scalar smax = std::numeric_limits<Scalar>::max();
   bMin = Vector(smax, smax, smax);
   bMax = Vector(-smax, -smax, -smax);

   scene = rtcNewScene(RTC_SCENE_STATIC|sceneFlags, intersections);

   if (geometry && geometry->hasAttribute("_color")) {

      std::stringstream str(geometry->getAttribute("_color"));
      char ch;
      str >> ch;
      if (ch == '#') {
         str << std::hex;
         unsigned long c = 0xffffffffL;
         str >> c;
         if (c > 0x00ffffffL) {
            c = 0x00ffffffL;
         }
         float b = (c & 0xffL);
         c >>= 8;
         float g = (c & 0xffL);
         c >>= 8;
         float r = (c & 0xffL);
         c >>= 8;

         hasSolidColor = true;
         solidColor = Vector4(r, g, b, 255.f);
      }
   }

   if (auto tri = Triangles::as(geometry)) {

      Index numElem = tri->getNumElements();
      if (numElem == 0) {
         numElem = tri->getNumCoords() / 3;
      }
      geomId = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, numElem, tri->getNumCoords());
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

      indexBuffer = new Triangle[numElem];
      rtcSetBuffer(scene, geomId, RTC_INDEX_BUFFER, indexBuffer, 0, sizeof(Triangle));
      Triangle* triangles = (Triangle*) rtcMapBuffer(scene,geomId,RTC_INDEX_BUFFER);
      if (tri->getNumElements() == 0) {
         for (Index i=0; i<numElem; ++i) {
            triangles[i].v0 = i*3;
            triangles[i].v1 = i*3+1;
            triangles[i].v2 = i*3+2;
         }
      } else {
         for (Index i=0; i<numElem; ++i) {
            triangles[i].v0 = tri->cl()[i*3];
            triangles[i].v1 = tri->cl()[i*3+1];
            triangles[i].v2 = tri->cl()[i*3+2];
         }
      }
      rtcUnmapBuffer(scene,geomId,RTC_INDEX_BUFFER);
   } else if (auto poly = Polygons::as(geometry)) {

      Index ntri = poly->getNumCorners()-2*poly->getNumElements();
      vassert(ntri >= 0);

      geomId = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, ntri, poly->getNumCoords());
      //std::cerr << "Poly: #tri: " << poly->getNumCorners()-2*poly->getNumElements() << ", #coord: " << poly->getNumCoords() << std::endl;

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
         const Index end = poly->el()[i+1];
         const Index nvert = end-start;
         const Index last = end-1;
         for (Index v=0; v<nvert-2; ++v) {
            const Index v2 = v/2;
            if (v%2) {
               triangles[t].v0 = poly->cl()[last-v2];
               triangles[t].v1 = poly->cl()[start+v2+1];
               triangles[t].v2 = poly->cl()[last-v2-1];
            } else {
               triangles[t].v0 = poly->cl()[start+v2];
               triangles[t].v1 = poly->cl()[last-v2];
               triangles[t].v2 = poly->cl()[start+v2+1];
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
   delete[] indexBuffer;
}
