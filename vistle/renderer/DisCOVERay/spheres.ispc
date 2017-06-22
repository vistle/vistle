// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

//#include "../common/tutorial/tutorial_device.isph"

#include "linalg/math.isph"
#include "linalg/vec.isph"
#include "linalg/affinespace.isph"

#include <embree2/rtcore.isph>
#include <embree2/rtcore_ray.isph>

#include "render.isph"
#include "spheres.isph"

#include "spheres_isect.isph"

#include "common.h"

// ======================================================================== //
//                     User defined sphere geometry                         //
// ======================================================================== //

unmasked void sphereBoundsFunc(void *uniform ptr, uniform item_t item, uniform RTCBounds &bounds_o)
{
  const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
  const uniform Sphere& sphere = data->spheres[item];
  bounds_o.lower_x = sphere.p.x-sphere.r;
  bounds_o.lower_y = sphere.p.y-sphere.r;
  bounds_o.lower_z = sphere.p.z-sphere.r;
  bounds_o.upper_x = sphere.p.x+sphere.r;
  bounds_o.upper_y = sphere.p.y+sphere.r;
  bounds_o.upper_z = sphere.p.z+sphere.r;
}

void sphereIntersectFunc(void *uniform ptr, varying RTCRay& ray, uniform item_t item)
{
  const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
  const uniform Sphere& sphere = data->spheres[item];
  if (sphereIsect(sphere, ray)) {
     ray.geomID = data->geomId;
     ray.primID = item;
  }
}

unmasked void sphereIntersectFuncN(const uniform int* uniform valid, /*! pointer to valid mask */
                                           void *uniform ptr,                /*!< pointer to geometry user data */
                                           const uniform RTCIntersectContext* uniform context,  /*!< intersection context as passed to rtcIntersect/rtcOccluded */
                                           RTCRayN* uniform rays,               /*!< ray packet of size N */
                                           uniform item_t N,                 /*< number of rays in ray packet */
                                           uniform item_t item              /*< item to intersect */)
{
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    const uniform Sphere& sphere = data->spheres[item];

    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui=0; ui<N; ui+=programCount)
    {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui+programIndex;
        if (vi>=N) continue;

        /* ignore inactive rays */
        if (valid[vi] != -1) continue;

        const Vec3f ray_org = make_Vec3f(RTCRayN_org_x(rays,N,ui),RTCRayN_org_y(rays,N,ui),RTCRayN_org_z(rays,N,ui));
        const Vec3f ray_dir = make_Vec3f(RTCRayN_dir_x(rays,N,ui),RTCRayN_dir_y(rays,N,ui),RTCRayN_dir_z(rays,N,ui));

        float tfar = RTCRayN_tfar(rays,N,ui);
        Vec3f ray_Ng;
        if (sphereIsect2(sphere, ray_org, ray_dir,
                         RTCRayN_tnear(rays,N,ui), tfar, ray_Ng)) {
            RTCRayN_geomID(rays,N,ui) = data->geomId;
            RTCRayN_primID(rays,N,ui) = item;
            RTCRayN_u(rays,N,ui) = 0.f;
            RTCRayN_v(rays,N,ui) = 0.f;
            RTCRayN_tfar(rays,N,ui) = tfar;
            RTCRayN_Ng_x(rays,N,ui) = ray_Ng.x;
            RTCRayN_Ng_y(rays,N,ui) = ray_Ng.y;
            RTCRayN_Ng_z(rays,N,ui) = ray_Ng.z;
        }
    }
}

void sphereOccludedFunc(void *uniform ptr, varying RTCRay& ray, uniform item_t item)
{
  const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
  const uniform Sphere& sphere = data->spheres[item];
  if (sphereOcclude(sphere, ray)) {
     ray.geomID = 0;
  }
}

unmasked void sphereOccludedFuncN(const uniform int* uniform valid, /*! pointer to valid mask */
                                           void *uniform ptr,                /*!< pointer to geometry user data */
                                           const uniform RTCIntersectContext* uniform context,  /*!< intersection context as passed to rtcIntersect/rtcOccluded */
                                           RTCRayN* uniform rays,               /*!< ray packet of size N */
                                           uniform item_t N,                 /*< number of rays in ray packet */
                                           uniform item_t item              /*< item to intersect */)
{
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    const uniform Sphere& sphere = data->spheres[item];

    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui=0; ui<N; ui+=programCount)
    {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui+programIndex;
        if (vi>=N) continue;

        /* ignore inactive rays */
        if (valid[vi] != -1) continue;

        const Vec3f ray_org = make_Vec3f(RTCRayN_org_x(rays,N,ui),RTCRayN_org_y(rays,N,ui),RTCRayN_org_z(rays,N,ui));
        const Vec3f ray_dir = make_Vec3f(RTCRayN_dir_x(rays,N,ui),RTCRayN_dir_y(rays,N,ui),RTCRayN_dir_z(rays,N,ui));

        float tfar = RTCRayN_tfar(rays,N,ui);
        Vec3f ray_Ng;
        if (sphereOcclude2(sphere, ray_org, ray_dir,
                         RTCRayN_tnear(rays,N,ui), RTCRayN_tfar(rays,N,ui))) {
            RTCRayN_geomID(rays,N,ui) = 0;
        }
    }
}

export uniform unsigned int registerSpheres(uniform RTCScene scene, uniform RenderObjectData *uniform data, uniform size_t N) {

  assert(data->spheres);

  const uniform unsigned int geomID = rtcNewUserGeometry(scene,N);
  data->geomId = geomID;
  rtcSetUserData(scene, geomID, data);
  rtcSetBoundsFunction(scene, geomID, &sphereBoundsFunc);
#ifdef USE_STREAM
  rtcSetIntersectFunctionN(scene, geomID, &sphereIntersectFuncN);
  rtcSetOccludedFunctionN(scene, geomID, &sphereOccludedFuncN);
#else
  rtcSetIntersectFunction(scene, geomID, &sphereIntersectFunc);
  rtcSetOccludedFunction(scene, geomID, &sphereOccludedFunc);
#endif
  return geomID;
}
