#include "render.isph"

#include <embree2/rtcore.isph>
#include <embree2/rtcore_ray.isph>

#include "linalg/math.isph"
#include "linalg/vec.isph"

#include "common.h"

inline uniform float dot(const uniform Vec4f a, const uniform Vec4f b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
inline varying float dot(const varying Vec4f a, const varying Vec4f b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }

inline void setUpRay(uniform const TileData *uniform tile,
                     varying RTCRay &ray, const varying int x, const varying int y) {

    const Vec3f origin = tile->origin;

    ray.orgx = origin.x;
    ray.orgy = origin.y;
    ray.orgz = origin.z;

    const Vec3f rdir = tile->corner + x*tile->dx + y*tile->dy;
    ray.dirx = rdir.x;
    ray.diry = rdir.y;
    ray.dirz = rdir.z;

    ray.tnear = tile->tNear;
    ray.tfar = tile->tFar;

    ray.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.primID = RTC_INVALID_GEOMETRY_ID;
    ray.instID = RTC_INVALID_GEOMETRY_ID;

    ray.mask = RayEnabled;
    ray.time = 0.0f;
}


inline void shadeRay(uniform const SceneData *uniform scene, uniform const TileData *uniform tile,
                     const varying RTCRay &ray, const varying int x, const varying int y) {

    uniform const float ambientFactor = 0.2f;
    uniform const Vec4f specColor = make_Vec4f(0.4f, 0.4f, 0.4f, 1.0f);
    uniform const float specExp = 16.f;
    uniform const Vec4f ambient = make_Vec4f(0.2f, 0.2f, 0.2f, 1.0f);
    uniform const bool twoSided = true;

    const Vec3f origin = tile->origin;
    const Vec4f depthTransform2 = tile->depthTransform2;
    const Vec4f depthTransform3 = tile->depthTransform3;

    Vec4f shaded = make_Vec4f(0);
    float zValue = 1.f;
    if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {

        const Vec3f rdir = make_Vec3f(ray.dirx,ray.diry,ray.dirz);
        const Vec3f pos = origin + ray.tfar * rdir;
        Vec3f dir = normalize(rdir);
        const Vec4f pos4 = make_Vec4f(pos.x, pos.y, pos.z, 1);
        const float win2 = dot(depthTransform2, pos4);
        const float win3 = dot(depthTransform3, pos4);
        zValue= (win2/win3+1.f)*0.5f;

        cif (scene->doShade) {

            RenderObjectData *ro = scene->ro[ray.instID];
            assert(ro->geomId == ray.geomID);

            Vec4f color = scene->defaultColor;
            if (ro->hasSolidColor) {
                color = make_Vec4f(ro->solidColor[0]*255.f, ro->solidColor[1]*255.f, ro->solidColor[2]*255.f, ro->solidColor[3]*255.f);
            }
            if (scene->uvVis) {
                const float &u = ray.u;
                const float &v = ray.v;
                const float w = 1.f - u - v;
                color.x = u*255.f;
                color.y = v*255.f;
                color.z = w*255.f;
                color.w = 255.f;
            }
            else if (ro->texData && ro->texCoords) {

                const float &u = ray.u;
                const float &v = ray.v;
                const float w = 1.f - u - v;
                unsigned idx = 0;
                if (ro->perPrimitiveMapping) {
                    if (ro->indexBuffer) {
                        const float tc = clamp(ro->texCoords[ro->indexBuffer[ray.primID].elem], 0.f, 1.f);
                        idx = min(tc * ro->texWidth, ro->texWidth-1);
                    } else {
                        const float tc = clamp(ro->texCoords[ray.primID], 0.f, 1.f);
                        idx = min(tc * ro->texWidth, ro->texWidth-1);
                    }
                } else if (ro->indexBuffer) {
                    const unsigned int v0 = ro->indexBuffer[ray.primID].v0;
                    const unsigned int v1 = ro->indexBuffer[ray.primID].v1;
                    const unsigned int v2 = ro->indexBuffer[ray.primID].v2;

                    const float tc0 = ro->texCoords[v0];
                    const float tc1 = ro->texCoords[v1];
                    const float tc2 = ro->texCoords[v2];
                    const float tc = clamp(w*tc0 + u*tc1 + v*tc2, 0.f, 1.f);
                    idx = min(tc * ro->texWidth, ro->texWidth-1);
                } else {
                    const unsigned int v0 = ray.primID;
                    const unsigned int v1 = ray.primID + 1;
                    const float tc0 = ro->texCoords[v0];
                    const float tc1 = ro->texCoords[v1];
                    const float tc = clamp(w*tc0 + u*tc1, 0.f, 1.f);
                    idx = min(tc * ro->texWidth, ro->texWidth-1);
                }
                const unsigned int8 *c = &ro->texData[idx*4];
                color.x = c[0];
                color.y = c[1];
                color.z = c[2];
                color.w = c[3];
            }

            if (ro->lighted) {
                Vec4f ambientColor = color;
                ambientColor.x *= ambientFactor;
                ambientColor.y *= ambientFactor;
                ambientColor.z *= ambientFactor;
                Vec3f normal;
                if (ro->normals[0]) {
                    const float &u = ray.u;
                    const float &v = ray.v;
                    const float w = 1.f - u - v;
                    if (ro->normalsPerPrimitiveMapping) {
                        if (ro->indexBuffer) {
                            const unsigned int idx = ro->indexBuffer[ray.primID].elem;
                            normal = normalize(make_Vec3f(ro->normals[0][idx], ro->normals[1][idx], ro->normals[2][idx]));
                        } else {
                            const unsigned int idx = ray.primID;
                            normal = normalize(make_Vec3f(ro->normals[0][idx], ro->normals[1][idx], ro->normals[2][idx]));
                        }
                    } else if (ro->indexBuffer) {
                        const unsigned int v0 = ro->indexBuffer[ray.primID].v0;
                        const unsigned int v1 = ro->indexBuffer[ray.primID].v1;
                        const unsigned int v2 = ro->indexBuffer[ray.primID].v2;
                        const Vec3f n0 = make_Vec3f(ro->normals[0][v0], ro->normals[1][v0], ro->normals[2][v0]);
                        const Vec3f n1 = make_Vec3f(ro->normals[0][v1], ro->normals[1][v1], ro->normals[2][v1]);
                        const Vec3f n2 = make_Vec3f(ro->normals[0][v2], ro->normals[1][v2], ro->normals[2][v2]);
                        normal = normalize(w*n0 + u*n1 + v*n2);
                    } else {
                        const unsigned int v0 = ray.primID;
                        const unsigned int v1 = ray.primID + 1;
                        const Vec3f n0 = make_Vec3f(ro->normals[0][v0], ro->normals[1][v0], ro->normals[2][v0]);
                        const Vec3f n1 = make_Vec3f(ro->normals[0][v1], ro->normals[1][v1], ro->normals[2][v1]);
                        normal = normalize(w*n0 + u*n1);
                    }
                } else {
                    const Vec3f Ng = make_Vec3f(ray.Ngx,ray.Ngy,ray.Ngz);
                    normal = normalize(Ng);
                }
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
            } else {
                shaded = color;
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


inline void ispcRenderTilePacket(uniform const SceneData *uniform scene, uniform const TileData *uniform tile) {

   uniform const int x0=tile->x0, x1=tile->x1;
   uniform const int y0=tile->y0, y1=tile->y1;

   int x,y;
   foreach_tiled(y=y0...y1, x=x0...x1) {

      RTCRay ray;
      setUpRay(tile, ray, x, y);
      rtcIntersect(scene->scene, ray);
      shadeRay(scene, tile, ray, x, y);
   }
}

inline void ispcRenderTileStream(uniform const SceneData *uniform scene, uniform const TileData *uniform tile) {

   uniform const int x0=tile->x0, x1=tile->x1;
   uniform const int y0=tile->y0, y1=tile->y1;

   bool valid_stream[TILESIZE*TILESIZE];
   RTCRay primary_stream[TILESIZE*TILESIZE];

   int x, y;
   /* set up rays */
   uniform int N=0;
   foreach_tiled(y=y0...y1, x=x0...x1) {
      /* ISPC workaround for mask == 0 */
      if (all(__mask == 0)) continue;

      bool mask = __mask; unmasked { valid_stream[N] = mask; }
      RTCRay &ray = primary_stream[N];
      setUpRay(tile, ray, x, y);
      mask = __mask; unmasked { // invalidates inactive rays
          ray.tnear = mask ? 0.0f         : (float)(pos_inf);
          ray.tfar  = mask ? (float)(inf) : (float)(neg_inf);
      }

      ++N;
   }

   /* shoot rays */
   uniform RTCIntersectContext primary_context;
   primary_context.flags = RTC_INTERSECT_COHERENT;
   primary_context.userRayExt = &primary_stream;
   rtcIntersectVM(scene->scene,&primary_context,(varying RTCRay* uniform)&primary_stream,N,sizeof(RTCRay));

   /* shade rays */
   N=-1;
   foreach_tiled(y=y0...y1, x=x0...x1) {
      ++N;
      /* ISPC workaround for mask == 0 */
      if (all(__mask == 0)) continue;

      /* ignore invalid rays */
      if (valid_stream[N] == false) continue;

      RTCRay &ray = primary_stream[N];
      shadeRay(scene, tile, ray, x, y);
   }
}

export void ispcRenderTile(uniform const SceneData *uniform scene, uniform const TileData *uniform tile) {
#ifdef USE_STREAM
    ispcRenderTileStream(scene, tile);
#else
    ispcRenderTilePacket(scene, tile);
#endif
}
