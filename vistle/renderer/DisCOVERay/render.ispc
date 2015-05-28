#include "render.isph"

#include <embree2/rtcore.isph>
#include <embree2/rtcore_ray.isph>

#include "linalg/math.isph"
#include "linalg/vec3f.isph"
#include "linalg/vec4f.isph"

#include "common.h"

inline uniform float dot(const uniform Vec4f a, const uniform Vec4f b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
inline varying float dot(const varying Vec4f a, const varying Vec4f b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }



static void makeRay(RTCRay &ray, uniform const float origin[3], uniform const float direction[3],
      uniform float tNear, uniform float tFar) {

}

export void ispcRenderTile(uniform const SceneData *uniform scene, uniform const TileData *uniform tile) {

   uniform const float ambientFactor = 0.2f;
   uniform const Vec4f specColor = make_Vec4f(0.4f, 0.4f, 0.4f, 1.0f);
   uniform const float specExp = 16.f;
   uniform const Vec4f ambient = make_Vec4f(0.2f, 0.2f, 0.2f, 1.0f);
   uniform const bool twoSided = true;

   uniform const int x0=tile->x0, x1=tile->x1;
   uniform const int y0=tile->y0, y1=tile->y1;

   const Vec3f origin = tile->origin;
   const Vec4f depthTransform2 = tile->depthTransform2;
   const Vec4f depthTransform3 = tile->depthTransform3;

   int x, y;
   foreach_tiled(y=y0...y1, x=x0...x1) {

      RTCRay ray;
      ray.orgx = origin.x;
      ray.orgy = origin.y;
      ray.orgz = origin.z;

      Vec3f dir = tile->corner + x*tile->dx + y*tile->dy;
      ray.dirx = dir.x;
      ray.diry = dir.y;
      ray.dirz = dir.z;

      ray.tnear = tile->tNear;
      ray.tfar = tile->tFar;

      ray.geomID = RTC_INVALID_GEOMETRY_ID;
      ray.primID = RTC_INVALID_GEOMETRY_ID;
      ray.instID = RTC_INVALID_GEOMETRY_ID;

      ray.mask = RayEnabled;
      ray.time = 0.0f;

      rtcIntersect(scene->scene, ray);

      Vec4f shaded = make_Vec4f(0);
      float zValue = 1.f;
      if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {

         const Vec3f pos = origin + ray.tfar * dir;
         dir = normalize(dir);
         const Vec4f pos4 = make_Vec4f(pos.x, pos.y, pos.z, 1);
         const float win2 = dot(depthTransform2, pos4);
         const float win3 = dot(depthTransform3, pos4);
         zValue= (win2/win3+1.f)*0.5f;

         cif (scene->doShade) {

            RenderObjectData *ro = scene->ro[ray.instID];
            assert(ro->geomId == ray.geomID);

            Vec4f color = scene->defaultColor;
            if (ro->hasSolidColor) {
               color = make_Vec4f(ro->solidColor[0], ro->solidColor[1], ro->solidColor[2], ro->solidColor[3]);
            }
            if (ro->indexBuffer && ro->texData && ro->texCoords) {

               const float &u = ray.u;
               const float &v = ray.v;
               const float w = 1.f - u - v;
               const unsigned int v0 = ro->indexBuffer[ray.primID].v0;
               const unsigned int v1 = ro->indexBuffer[ray.primID].v1;
               const unsigned int v2 = ro->indexBuffer[ray.primID].v2;

               const float tc0 = ro->texCoords[v0];
               const float tc1 = ro->texCoords[v1];
               const float tc2 = ro->texCoords[v2];
               const float tc = clamp(w*tc0 + u*tc1 + v*tc2, 0.f, 1.f);
               const unsigned idx = min(tc * ro->texWidth, ro->texWidth-1);
               const unsigned int8 *c = &ro->texData[idx*4];
               color.x = c[0];
               color.y = c[1];
               color.z = c[2];
               color.w = c[3];
            }

            Vec4f ambientColor = color;
            ambientColor.x *= ambientFactor;
            ambientColor.y *= ambientFactor;
            ambientColor.z *= ambientFactor;
            Vec3f normal = normalize(make_Vec3f(ray.Ngx, ray.Ngy, ray.Ngz));
            if (twoSided && dot(normal, dir) > 0.f)
               normal = neg(normal);

            shaded = shaded + ambientColor * ambient;
            cfor (uniform int l=0; l<scene->numLights; ++l) {
               const uniform Light &light = scene->lights[l];
               cif (light.enabled) {
                  const Vec3f lv = normalize(light.isDirectional ? light.transformedPosition : light.transformedPosition-pos);
                  float atten = 1.f;
                  cif (light.isDirectional) {
                     atten = light.attenuation[0];
                     cif (light.attenuation[1]>0.f || light.attenuation[2]>0.f) {
                        Vec4f ec = make_Vec4f(light.transformedPosition-pos);
                        float d = 0.;
                        for (int i=0; i<3; ++i) {
                           float dd = dot(scene->modelView[i], ec);
                           d += dd*dd;
                        }
                        atten += (light.attenuation[1] + light.attenuation[2]*d)*d;
                     }
                     atten = rcp(atten);
                  }
                  shaded = shaded + ambientColor * atten * light.ambient;
                  const float ldot = max(0.f, dot(normal, lv));
                  shaded = shaded + color * atten *ldot * light.diffuse;
                  if (ldot > 0.f) {
                     const Vec3f halfway = normalize(lv-dir);
                     const float hdot = max(0.f, dot(normal, halfway));
                     if (hdot > 0.f) {
                        shaded = shaded + specColor * atten * powf(hdot, specExp) * light.specular;
                     }
                  }
               }
            }

            shaded.x = clamp(shaded.x, 0.f, 255.f);
            shaded.y = clamp(shaded.y, 0.f, 255.f);
            shaded.z = clamp(shaded.z, 0.f, 255.f);
            shaded.w = clamp(shaded.w, 0.f, 255.f);

         } else {

            shaded = scene->defaultColor;
         }
      }

      tile->depth[y*tile->imgWidth+x] = zValue;
      unsigned int8 *rgba = tile->rgba+(y*tile->imgWidth+x)*4;
      rgba[0] = shaded.x;
      rgba[1] = shaded.y;
      rgba[2] = shaded.z;
      rgba[3] = shaded.w;
   }
}
