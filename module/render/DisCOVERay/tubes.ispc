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

#include "common.h"

#include EMBREE(rtcore.isph)
#include EMBREE(rtcore_ray.isph)

#include "render.isph"
#include "tubes.isph"

#include "spheres_isect.isph"

uniform const float Epsilon = 1e-6;
uniform const float ArrowSize = 2.0f;

bool coneSectionIntersectN(const uniform TubeSection &section0, const uniform TubeSection &section1, uniform int N,
                           uniform int i, RTCRayN *uniform ray, RTCHitN *uniform hit)
{
    uniform Vec3f coneDir = section1.p - section0.p;
    uniform const float coneLength = length(coneDir);
    if (coneLength <= Epsilon)
        return false;
    const uniform float coneLengthInv = rcp(coneLength);
    uniform const float AbsTan = abs(section1.r - section0.r) * coneLengthInv;
    uniform const float Tan2 = sqr(AbsTan);
    uniform bool cylinder = true;
    if (Tan2 > 10.f * Epsilon && abs(section0.r - section1.r) > 100.f * Epsilon && coneLength > 100.f * Epsilon)
        cylinder = false;
    uniform const float Cos2 = cylinder ? 1.f : rcp(1.f + Tan2);

    const Vec3f dir = make_Vec3f(RTCRayN_dir_x(ray, N, i), RTCRayN_dir_y(ray, N, i), RTCRayN_dir_z(ray, N, i));
    const float rayLenInv = rcp_safe(length(dir));
    const Vec3f rayDir = dir * rayLenInv;
    uniform Vec3f tip = section0.p; // tip of cone
    uniform Vec3f top = tip; // top of cone section
    uniform float r = section0.r;
    // after this, coneDir goes from tip to bottom
    uniform bool widening = false;
    if (!cylinder) {
        if (section0.r > section1.r) {
            widening = false;
            r = section1.r;
            top = section1.p;
            tip = top + coneDir * r / (section0.r - section1.r);
            coneDir = neg(coneDir);
        } else {
            widening = true;
            r = section0.r;
            top = section0.p;
            tip = top + coneDir * r / (section0.r - section1.r);
        }
    }
    coneDir = normalize(coneDir);
    uniform const Vec3f M0 = make_Vec3f(coneDir.x * coneDir.x - Cos2, coneDir.x * coneDir.y, coneDir.x * coneDir.z);
    uniform const Vec3f M1 = make_Vec3f(coneDir.x * coneDir.y, coneDir.y * coneDir.y - Cos2, coneDir.y * coneDir.z);
    uniform const Vec3f M2 = make_Vec3f(coneDir.x * coneDir.z, coneDir.y * coneDir.z, coneDir.z * coneDir.z - Cos2);
    const Vec3f Mxdir = make_Vec3f(dot(M0, rayDir), dot(M1, rayDir), dot(M2, rayDir));
    const float c2 = dot(rayDir, Mxdir);
    if (abs(c2) < Epsilon) {
        // on cone boundary
        return false;
    }
    const Vec3f org = make_Vec3f(RTCRayN_org_x(ray, N, i), RTCRayN_org_y(ray, N, i), RTCRayN_org_z(ray, N, i));
    const Vec3f Delta = org - tip;
    const Vec3f MxDelta = make_Vec3f(dot(M0, Delta), dot(M1, Delta), dot(M2, Delta));
    const float c1 = dot(rayDir, MxDelta);
    float c0 = dot(Delta, MxDelta);
    if (cylinder) {
        c0 += sqr(r);
    }
    const float disc = c1 * c1 - c0 * c2;
    if (disc < 0) {
        // no intersection
        return false;
    }

    const float sqrtDisc = sqrt(disc);
    const float c2Inv = rcp(c2);
    const float t0 = (-c1 + sqrtDisc) * c2Inv * rayLenInv;
    const float t1 = (-c1 - sqrtDisc) * c2Inv * rayLenInv;

    //print("|n=% f=% 0=% 1=%|.", ray.tnear, ray.tfar, t0, t1);

    float &tnear = RTCRayN_tnear(ray, N, i);
    float &tfar = RTCRayN_tfar(ray, N, i);
    bool haveHit = false;
    if ((tnear < t0) & (t0 < tfar)) {
        const Vec3f pRel = org + t0 * dir - top;
        const float topDist = dot(coneDir, pRel);
        if (topDist >= 0 && topDist <= coneLength) {
            haveHit = true;
            tfar = t0;
            if (hit) {
                const Vec3f pRelProj = coneDir * topDist;

                const Vec3f Ng = normalize(pRel - pRelProj) - coneDir * AbsTan;
                RTCHitN_Ng_x(hit, N, i) = Ng.x;
                RTCHitN_Ng_y(hit, N, i) = Ng.y;
                RTCHitN_Ng_z(hit, N, i) = Ng.z;
                float u = widening ? topDist * coneLengthInv : 1.f - topDist * coneLengthInv;
                RTCHitN_u(hit, N, i) = u;
                RTCHitN_v(hit, N, i) = cylinder ? 0.0f : 1.f - u;
            }
        }
    }
    if ((tnear < t1) & (t1 < tfar)) {
        const Vec3f pRel = org + t1 * dir - top;
        const float topDist = dot(coneDir, pRel);
        if (topDist >= 0 && topDist <= coneLength) {
            haveHit = true;
            tfar = t1;
            if (hit) {
                const Vec3f pRelProj = coneDir * topDist;

                const Vec3f Ng = normalize(pRel - pRelProj) - coneDir * AbsTan;
                RTCHitN_Ng_x(hit, N, i) = Ng.x;
                RTCHitN_Ng_y(hit, N, i) = Ng.y;
                RTCHitN_Ng_z(hit, N, i) = Ng.z;
                float u = widening ? topDist * coneLengthInv : 1.f - topDist * coneLengthInv;
                RTCHitN_u(hit, N, i) = u;
                RTCHitN_v(hit, N, i) = cylinder ? 0.0f : 1.f - u;
            }
        }
    }

    return haveHit;
}

inline bool discIntersectN(const uniform TubeSection &center, const uniform Vec3f &normal, uniform int N, uniform int i,
                           RTCRayN *uniform ray, RTCHitN *uniform hit)
{
    const Vec3f dir = make_Vec3f(RTCRayN_dir_x(ray, N, i), RTCRayN_dir_y(ray, N, i), RTCRayN_dir_z(ray, N, i));
    const float prod = dot(normal, dir);
    if (abs(prod) < Epsilon) {
        // ray and disc are parallel
        return false;
    }

    const Vec3f org = make_Vec3f(RTCRayN_org_x(ray, N, i), RTCRayN_org_y(ray, N, i), RTCRayN_org_z(ray, N, i));
    const float t = dot(normal, center.p - org) / prod;
    float &tnear = RTCRayN_tnear(ray, N, i);
    float &tfar = RTCRayN_tfar(ray, N, i);
    if ((tnear < t) && (t < tfar)) {
        const Vec3f p = org + t * dir;
        if (length(center.p - p) <= center.r) {
            if (hit) {
                RTCHitN_Ng_x(hit, N, i) = normal.x;
                RTCHitN_Ng_y(hit, N, i) = normal.y;
                RTCHitN_Ng_z(hit, N, i) = normal.z;
                RTCHitN_u(hit, N, i) = 0.f;
                RTCHitN_v(hit, N, i) = 0.f;
            }
            tfar = t;

            return true;
        }
    }
    return false;
}

struct Plane {
    Vec3f normal;
    float dist;
};

inline bool sphereSectorIntersectN(const uniform Sphere &sphere, const uniform Plane &plane0,
                                   const uniform Plane &plane1, uniform int N, uniform int i, RTCRayN *uniform ray,
                                   RTCHitN *uniform hit)
{
    const Vec3f dir = make_Vec3f(RTCRayN_dir_x(ray, N, i), RTCRayN_dir_y(ray, N, i), RTCRayN_dir_z(ray, N, i));
    const Vec3f org = make_Vec3f(RTCRayN_org_x(ray, N, i), RTCRayN_org_y(ray, N, i), RTCRayN_org_z(ray, N, i));
    const Vec3f v = org - sphere.p;
    const float A = dot(dir, dir);
    const float B = dot(v, dir);
    const float C = dot(v, v) - sqr(sphere.r);
    const float D = B * B - A * C;
    if (D < 0.0f)
        return false;
    const float Q = sqrt(D);
    const float rcpA = rcp(A);
    const float t0 = rcpA * (-B - Q);
    const float t1 = rcpA * (-B + Q);
    bool haveHit = false;
    float &tnear = RTCRayN_tnear(ray, N, i);
    float &tfar = RTCRayN_tfar(ray, N, i);
    float t = tfar;
    if ((tnear < t0) && (t0 < tfar)) {
        haveHit = true;
        t = t0;
    }
    if ((tnear < t1) & (t1 < t)) {
        haveHit = true;
        t = t1;
    }
    if (haveHit) {
        const Vec3f p = org + t * dir;
        if (dot(plane0.normal, p) < plane0.dist - Epsilon) {
            return false;
        }
        if (dot(plane1.normal, p) < plane1.dist - Epsilon) {
            return false;
        }
        tfar = t;
        if (hit) {
            const Vec3f Ng = p - sphere.p;
            RTCHitN_Ng_x(hit, N, i) = Ng.x;
            RTCHitN_Ng_y(hit, N, i) = Ng.y;
            RTCHitN_Ng_z(hit, N, i) = Ng.z;
            RTCHitN_u(hit, N, i) = 1.f; // at end of tube section
            RTCHitN_v(hit, N, i) = 0.f;
        }
    }
    return haveHit;
}


// ======================================================================== //
//                     User defined tube geometry                         //
// ======================================================================== //

unmasked void tubeBoundsFunc(const struct RTCBoundsFunctionArguments *uniform args)
{
    void *uniform ptr = args->geometryUserPtr;
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    uniform unsigned int item = args->primID;
    const uniform unsigned int flags = data->primitiveFlags[item];
    if (flags == PFNone) {
        return;
    }

    const uniform unsigned int Start = PFCone | PFStartSphere | PFStartDisc;
    const uniform unsigned int End = PFCone | PFEndSphere | PFEndSphereSect | PFEndDisc | PFArrow;

    const uniform TubeSection &section0 = data->spheres[item];
    const uniform float r1 = section0.r;
    const uniform TubeSection &section1 = data->spheres[item + 1];
    const uniform float r2 = (flags & PFArrow) ? ArrowSize * section1.r : section1.r;

    uniform Vec3f lower, upper;
    if (flags & Start) {
        if (flags & End) {
            lower = min(section0.p - make_Vec3f(r1), section1.p - make_Vec3f(r2));
            upper = max(section0.p + make_Vec3f(r1), section1.p + make_Vec3f(r2));
        } else {
            lower = section0.p - make_Vec3f(r1);
            upper = section0.p + make_Vec3f(r1);
        }
    } else {
        lower = section1.p - make_Vec3f(r2);
        upper = section1.p + make_Vec3f(r2);
    }

    uniform RTCBounds *uniform bounds_o = args->bounds_o;
    bounds_o->lower_x = lower.x;
    bounds_o->lower_y = lower.y;
    bounds_o->lower_z = lower.z;
    bounds_o->upper_x = upper.x;
    bounds_o->upper_y = upper.y;
    bounds_o->upper_z = upper.z;
}

unmasked void tubeIntersectFuncN(const RTCIntersectFunctionNArguments *uniform args)
{
    uniform int *uniform valid = args->valid;
    void *uniform ptr = args->geometryUserPtr;
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    uniform unsigned int N = args->N;
    RTCRayHitN *uniform rayhit = args->rayhit;
    RTCRayN *uniform rays = RTCRayHitN_RayN(rayhit, N);
    RTCHitN *uniform hits = RTCRayHitN_HitN(rayhit, N);
    uniform unsigned int primID = args->primID;
    uniform unsigned int item = primID;

    const uniform TubeSection &section0 = data->spheres[item];
    const uniform TubeSection &section1 = data->spheres[item + 1];
    const uniform unsigned int flags = data->primitiveFlags[item];

#define HIT \
    RTCHitN_geomID(hits, N, ui) = data->geomID; \
    RTCHitN_primID(hits, N, ui) = primID; \
    RTCHitN_instID(hits, N, ui, 0) = args->context->instID[0];


    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui = 0; ui < N; ui += programCount) {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui + programIndex;
        if (vi >= N)
            continue;

        /* ignore inactive rays */
        if (valid[vi] != -1)
            continue;

        if (flags & PFCone) {
            if (coneSectionIntersectN(section0, section1, N, ui, rays, hits)) {
                HIT;
            }
        }

        if (flags & PFStartSphere) {
            if (sphereIntersectN(section0, N, ui, rays, hits)) {
                HIT;
            }
        }
        if (flags & PFEndSphere) {
            if (sphereIntersectN(section1, N, ui, rays, hits)) {
                HIT;
            }
        }

        if (flags & PFEndSphereSect) {
            uniform Plane p0, p1;
            p0.normal = normalize(section1.p - section0.p);
            p0.dist = dot(p0.normal, section1.p);
            p1.normal = normalize(section1.p - data->spheres[item + 2].p);
            p1.dist = dot(p1.normal, section1.p);
            if (sphereSectorIntersectN(section1, p0, p1, N, ui, rays, hits)) {
                HIT;
            }
        }

        if (flags & PFStartDisc) {
            uniform Vec3f coneDir = normalize(section1.p - section0.p);
            if (discIntersectN(section0, coneDir, N, ui, rays, hits)) {
                HIT;
            }
        }
        if (flags & PFEndDisc) {
            uniform Vec3f coneDir = normalize(section0.p - section1.p);
            if (discIntersectN(section1, coneDir, N, ui, rays, hits)) {
                HIT;
            }
        }
        if (flags & PFArrow) {
            uniform Vec3f coneDir = normalize(section1.p - section0.p);
            uniform TubeSection a1, a2;
            a1.p = section1.p;
            a1.r = ArrowSize * section1.r;
            a2.p = section1.p + coneDir * a1.r;
            a2.r = 0.f;
            if (coneSectionIntersectN(a1, a2, N, ui, rays, hits)) {
                HIT;
                RTCHitN_u(hits, N, ui) = 0.f;
                RTCHitN_v(hits, N, ui) = 0.f;
            }
            if (discIntersectN(a1, coneDir, N, ui, rays, hits)) {
                HIT;
                RTCHitN_u(hits, N, ui) = 0.f;
                RTCHitN_v(hits, N, ui) = 0.f;
            }
        }
    }

#undef HIT
}

unmasked void tubeOccludedFuncN(const RTCOccludedFunctionNArguments *uniform args)
{
    uniform int *uniform valid = args->valid;
    void *uniform ptr = args->geometryUserPtr;
    const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
    uniform unsigned int N = args->N;
    RTCRayN *uniform rays = args->ray;
    uniform unsigned int primID = args->primID;
    uniform unsigned int item = primID;

    const uniform TubeSection &section0 = data->spheres[item];
    const uniform TubeSection &section1 = data->spheres[item + 1];
    const uniform unsigned int flags = data->primitiveFlags[item];

    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui = 0; ui < N; ui += programCount) {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui + programIndex;
        if (vi >= N)
            continue;

        /* ignore inactive rays */
        if (valid[vi] != -1)
            continue;

        if (flags & PFCone) {
            coneSectionIntersectN(section0, section1, N, ui, rays, NULL);
        }

        if (flags & PFStartSphere) {
            sphereIntersectN(section0, N, ui, rays, NULL);
        }
        if (flags & PFEndSphere) {
            sphereIntersectN(section1, N, ui, rays, NULL);
        }

        if (flags & PFEndSphereSect) {
            uniform Plane p0, p1;
            p0.normal = normalize(section1.p - section0.p);
            p0.dist = dot(p0.normal, section1.p);
            p1.normal = normalize(section1.p - data->spheres[item + 2].p);
            p1.dist = dot(p1.normal, section1.p);
            sphereSectorIntersectN(section1, p0, p1, N, ui, rays, NULL);
        }

        if (flags & PFStartDisc) {
            uniform Vec3f coneDir = normalize(section1.p - section0.p);
            discIntersectN(section0, coneDir, N, ui, rays, NULL);
        }
        if (flags & PFEndDisc) {
            uniform Vec3f coneDir = normalize(section0.p - section1.p);
            discIntersectN(section1, coneDir, N, ui, rays, NULL);
        }
        if (flags & PFArrow) {
            uniform Vec3f coneDir = normalize(section1.p - section0.p);
            uniform TubeSection a1, a2;
            a1.p = section1.p;
            a1.r = ArrowSize * section1.r;
            a2.p = section1.p + coneDir * a1.r;
            a2.r = 0.f;
            coneSectionIntersectN(a1, a2, N, ui, rays, NULL);
            discIntersectN(a1, coneDir, N, ui, rays, NULL);
        }
    }
}

export uniform RTCGeometry newTubes(uniform RenderObjectData *uniform data, uniform uintptr_t numPrim)
{
    assert(data->spheres);

    uniform RTCGeometry geom = rtcNewGeometry(data->device, RTC_GEOMETRY_TYPE_USER);
    rtcSetGeometryBuildQuality(geom, RTC_BUILD_QUALITY_MEDIUM);
    rtcSetGeometryUserPrimitiveCount(geom, numPrim);
    rtcSetGeometryTimeStepCount(geom, 1);
    rtcSetGeometryUserData(geom, data);

    rtcSetGeometryBoundsFunction(geom, &tubeBoundsFunc, NULL);
    rtcSetGeometryIntersectFunction(geom, &tubeIntersectFuncN);
    rtcSetGeometryOccludedFunction(geom, &tubeOccludedFuncN);

    return geom;
}
