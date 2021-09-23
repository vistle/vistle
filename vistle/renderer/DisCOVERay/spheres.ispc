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

#include <embree3/rtcore.isph>
#include <embree3/rtcore_ray.isph>

#include "render.isph"
#include "spheres.isph"

#include "spheres_isect.isph"

#include "common.h"

// ======================================================================== //
//                     User defined sphere geometry                         //
// ======================================================================== //

unmasked void sphereBoundsFunc(const struct RTCBoundsFunctionArguments *uniform args)
{
    void *uniform ptr = args->geometryUserPtr;
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    const uniform Sphere &sphere = data->spheres[args->primID];
    uniform RTCBounds *uniform bounds_o = args->bounds_o;
    bounds_o->lower_x = sphere.p.x - sphere.r;
    bounds_o->lower_y = sphere.p.y - sphere.r;
    bounds_o->lower_z = sphere.p.z - sphere.r;
    bounds_o->upper_x = sphere.p.x + sphere.r;
    bounds_o->upper_y = sphere.p.y + sphere.r;
    bounds_o->upper_z = sphere.p.z + sphere.r;
}

unmasked void sphereIntersectFuncN(const RTCIntersectFunctionNArguments *uniform args)
{
    uniform int *uniform valid = args->valid;
    void *uniform ptr = args->geometryUserPtr;
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    uniform unsigned int N = args->N;
    RTCRayHitN *uniform rayhit = args->rayhit;
    RTCRayN *uniform rays = RTCRayHitN_RayN(rayhit, N);
    RTCHitN *uniform hits = RTCRayHitN_HitN(rayhit, N);
    uniform unsigned int primID = args->primID;
    const uniform Sphere &sphere = data->spheres[primID];

    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui = 0; ui < N; ui += programCount) {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui + programIndex;
        if (vi >= N)
            continue;

        /* ignore inactive rays */
        if (valid[vi] != -1)
            continue;

        if (sphereIntersectN(sphere, N, ui, rays, hits)) {
            RTCHitN_geomID(hits, N, ui) = data->geomID;
            RTCHitN_primID(hits, N, ui) = primID;
            RTCHitN_instID(hits, N, ui, 0) = args->context->instID[0];
        }
    }
}

unmasked void sphereOccludedFuncN(const RTCOccludedFunctionNArguments *uniform args)
{
    uniform int *uniform valid = args->valid;
    void *uniform ptr = args->geometryUserPtr;
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    uniform unsigned int N = args->N;
    RTCRayN *uniform ray = args->ray;
    uniform unsigned int primID = args->primID;
    const uniform Sphere &sphere = data->spheres[primID];

    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui = 0; ui < N; ui += programCount) {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui + programIndex;
        if (vi >= N)
            continue;

        /* ignore inactive rays */
        if (valid[vi] != -1)
            continue;

        sphereIntersectN(sphere, N, ui, ray, NULL);
    }
}

export uniform RTCGeometry newSpheres(uniform RenderObjectData *uniform data, uniform uintptr_t numPrim)
{
    assert(data->spheres);

    uniform RTCGeometry geom = rtcNewGeometry(data->device, RTC_GEOMETRY_TYPE_USER);
    rtcSetGeometryBuildQuality(geom, RTC_BUILD_QUALITY_MEDIUM);
    rtcSetGeometryUserPrimitiveCount(geom, numPrim);
    rtcSetGeometryTimeStepCount(geom, 1);
    rtcSetGeometryUserData(geom, data);

    rtcSetGeometryBoundsFunction(geom, &sphereBoundsFunc, NULL);
    rtcSetGeometryIntersectFunction(geom, &sphereIntersectFuncN);
    rtcSetGeometryOccludedFunction(geom, &sphereOccludedFuncN);

    return geom;
}
