#include <core/polygons.h>
#include <core/triangles.h>
#include <core/lines.h>
#include <core/tubes.h>
#include <core/spheres.h>
#include <core/points.h>

#include <core/assert.h>

#include <embree2/rtcore.h>

#include "rayrenderobject.h"
#include "spheres_ispc.h"
#include "tubes_ispc.h"

using namespace vistle;

using ispc::Vertex;
using ispc::Triangle;

float RayRenderObject::pointSize = 0.001f;

RayRenderObject::RayRenderObject(int senderId, const std::string &senderPort,
      Object::const_ptr container,
      Object::const_ptr geometry,
      Object::const_ptr normals,
      Object::const_ptr colors,
      Object::const_ptr texture)
: vistle::RenderObject(senderId, senderPort, container, geometry, normals, colors, texture)
, data(new ispc::RenderObjectData)
{
   data->scene = nullptr;
   data->geomId = RTC_INVALID_GEOMETRY_ID;
   data->instId = RTC_INVALID_GEOMETRY_ID;
   data->spheres = nullptr;
   data->primitiveFlags = nullptr;
   data->indexBuffer = nullptr;
   data->texWidth = 0;
   data->texData = nullptr;
   data->texCoords = nullptr;
   data->lighted = 1;
   data->hasSolidColor = hasSolidColor;
   for (int c=0; c<4; ++c) {
      data->solidColor[c] = solidColor[c];
   }
   if (this->texture) {
      data->texWidth = this->texture->getWidth();
      data->texData = this->texture->pixels().data();
      data->texCoords = &this->texture->coords()[0];
   }

   if (geometry->isEmpty()) {
      return;
   }

   data->scene = rtcNewScene(RTC_SCENE_STATIC|sceneFlags, intersections);

   if (auto tri = Triangles::as(geometry)) {

      Index numElem = tri->getNumElements();
      if (numElem == 0) {
         numElem = tri->getNumCoords() / 3;
      }
      data->geomId = rtcNewTriangleMesh(data->scene, RTC_GEOMETRY_STATIC, numElem, tri->getNumCoords());
      std::cerr << "Tri: #tri: " << tri->getNumElements() << ", #coord: " << tri->getNumCoords() << std::endl;

      Vertex* vertices = (Vertex*) rtcMapBuffer(data->scene, data->geomId, RTC_VERTEX_BUFFER);
      for (Index i=0; i<tri->getNumCoords(); ++i) {
         vertices[i].x = tri->x()[i];
         vertices[i].y = tri->y()[i];
         vertices[i].z = tri->z()[i];
      }
      rtcUnmapBuffer(data->scene, data->geomId, RTC_VERTEX_BUFFER);

      data->indexBuffer = new Triangle[numElem];
      rtcSetBuffer(data->scene, data->geomId, RTC_INDEX_BUFFER, data->indexBuffer, 0, sizeof(Triangle));
      Triangle* triangles = (Triangle*) rtcMapBuffer(data->scene, data->geomId, RTC_INDEX_BUFFER);
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
      rtcUnmapBuffer(data->scene, data->geomId, RTC_INDEX_BUFFER);
   } else if (auto poly = Polygons::as(geometry)) {

      Index ntri = poly->getNumCorners()-2*poly->getNumElements();
      vassert(ntri >= 0);

      data->geomId = rtcNewTriangleMesh(data->scene, RTC_GEOMETRY_STATIC, ntri, poly->getNumCoords());
      //std::cerr << "Poly: #tri: " << poly->getNumCorners()-2*poly->getNumElements() << ", #coord: " << poly->getNumCoords() << std::endl;

      Vertex* vertices = (Vertex*) rtcMapBuffer(data->scene, data->geomId, RTC_VERTEX_BUFFER);
      for (Index i=0; i<poly->getNumCoords(); ++i) {
         vertices[i].x = poly->x()[i];
         vertices[i].y = poly->y()[i];
         vertices[i].z = poly->z()[i];
      }
      rtcUnmapBuffer(data->scene, data->geomId, RTC_VERTEX_BUFFER);

      data->indexBuffer = new Triangle[ntri];
      rtcSetBuffer(data->scene, data->geomId, RTC_INDEX_BUFFER, data->indexBuffer, 0, sizeof(Triangle));
      Triangle* triangles = (Triangle*) rtcMapBuffer(data->scene, data->geomId, RTC_INDEX_BUFFER);
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
               triangles[t].v1 = poly->cl()[start+v2+1];
               triangles[t].v2 = poly->cl()[last-v2];
            }
            ++t;
         }
      }
      vassert(t == ntri);
      rtcUnmapBuffer(data->scene, data->geomId, RTC_INDEX_BUFFER);
   } else if (auto sph = Spheres::as(geometry)) {

      Index nsph = sph->getNumSpheres();
      //std::cerr << "Spheres: #sph: " << nsph << std::endl;
      data->spheres = new ispc::Sphere[nsph];
      auto x = &sph->x()[0];
      auto y = &sph->y()[0];
      auto z = &sph->z()[0];
      auto r = &sph->r()[0];
      auto s = data->spheres;
      for (Index i=0; i<nsph; ++i) {
         s[i].p.x = x[i];
         s[i].p.y = y[i];
         s[i].p.z = z[i];
         s[i].r = r[i];
      }
      data->geomId = registerSpheres((ispc::__RTCScene *)data->scene, data.get(), nsph);
   } else if (auto point = Points::as(geometry)) {

      Index np = point->getNumPoints();
      //std::cerr << "Points: #sph: " << np << std::endl;
      data->spheres = new ispc::Sphere[np];
      auto x = &point->x()[0];
      auto y = &point->y()[0];
      auto z = &point->z()[0];
      auto s = data->spheres;
      for (Index i=0; i<np; ++i) {
         s[i].p.x = x[i];
         s[i].p.y = y[i];
         s[i].p.z = z[i];
         s[i].r = pointSize;
      }
      data->lighted = 0;
      data->geomId = registerSpheres((ispc::__RTCScene *)data->scene, data.get(), np);
   } else if (auto line = Lines::as(geometry)) {

      Index nStrips = line->getNumElements();
      Index nPoints = line->getNumCorners();
      std::cerr << "Lines: #strips: " << nStrips << ", #corners: " << nPoints << std::endl;
      data->primitiveFlags = new unsigned int[nPoints];
      data->spheres = new ispc::Sphere[nPoints];

      auto el = &line->el()[0];
      auto cl = &line->cl()[0];
      auto x = &line->x()[0];
      auto y = &line->y()[0];
      auto z = &line->z()[0];
      auto s = data->spheres;
      auto p = data->primitiveFlags;
      Index idx=0;
      for (Index strip=0; strip<nStrips; ++strip) {
         const Index begin = el[strip], end = el[strip+1];
         for (Index c=begin; c<end; ++c) {
            
            Index i = cl[c];
            s[idx].p.x = x[i];
            s[idx].p.y = y[i];
            s[idx].p.z = z[i];
            s[idx].r = pointSize;
            p[idx] = ispc::PFNone;
            if (c+1 != end)
               p[idx] |= ispc::PFCone;
            p[idx] |= ispc::PFStartSphere;
            ++idx;
         }
      }
      vassert(idx == nPoints);
      data->lighted = 0;
      data->geomId = registerTubes((ispc::__RTCScene *)data->scene, data.get(), nPoints-1);
   } else if (auto tube = Tubes::as(geometry)) {

      Index nStrips = tube->getNumTubes();
      Index nPoints = tube->getNumCoords();
      std::cerr << "Tubes: #strips: " << nStrips << ", #corners: " << nPoints << std::endl;
      data->primitiveFlags = new unsigned int[nPoints];
      data->spheres = new ispc::Sphere[nPoints];
      const Tubes::CapStyle startStyle = tube->startStyle(), jointStyle = tube->jointStyle(), endStyle = tube->endStyle();

      auto el = tube->components().data();
      auto x = &tube->x()[0];
      auto y = &tube->y()[0];
      auto z = &tube->z()[0];
      auto r = &tube->r()[0];
      auto s = data->spheres;
      auto p = data->primitiveFlags;
      Index idx=0;
      for (Index strip=0; strip<nStrips; ++strip) {
         const Index begin = el[strip], end = el[strip+1];
         vassert(idx == begin);
         for (Index i=begin; i<end; ++i) {
            
            s[idx].p.x = x[i];
            s[idx].p.y = y[i];
            s[idx].p.z = z[i];
            s[idx].r = r[i];

            p[idx] = ispc::PFNone;
            if (i == begin) {
               switch(startStyle) {
                  case Tubes::Flat:
                     p[idx] |= ispc::PFStartDisc;
                     break;
                  case Tubes::Round:
                     p[idx] |= ispc::PFStartSphere;
                     break;
                  default:
                     break;
               }
            } else if (i+1 != end) {
               if (jointStyle == Tubes::Flat) {
                  p[idx] |= ispc::PFStartDisc;
               }
            }

            if (i+1 != end) {
               p[idx] |= ispc::PFCone;

               switch((i+2==end) ? endStyle : jointStyle) {
                  case Tubes::Open:
                     break;
                  case Tubes::Flat:
                     p[idx] |= ispc::PFEndDisc;
                     break;
                  case Tubes::Round:
                     p[idx] |= i+2==end ? ispc::PFEndSphere : ispc::PFEndSphereSect;
                     break;
                  case Tubes::Arrow:
                     p[idx] |= ispc::PFArrow;
                     break;
               }
            }

            ++idx;
         }
         vassert(idx == end);
      }
      vassert(idx == nPoints);
      data->geomId = registerTubes((ispc::__RTCScene *)data->scene, data.get(), nPoints > 0 ? nPoints-1 : 0);
   }

   rtcCommit(data->scene);
}

RayRenderObject::~RayRenderObject() {

   delete[] data->spheres;
   delete[] data->primitiveFlags;
   //rtcDeleteGeometry(data->scene, data->geomId); // not possible for static geometry
   if (data->scene)
      rtcDeleteScene(data->scene);
   delete[] data->indexBuffer;
}
