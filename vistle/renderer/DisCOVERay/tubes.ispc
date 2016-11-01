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
#include "tubes.isph"

#include "spheres_isect.isph"

uniform const float Epsilon = 1e-6;
uniform const float ArrowSize = 1.5f;

bool coneSectionIsect(const uniform TubeSection &section0,
      const uniform TubeSection &section1,
      varying RTCRay &ray) {

  uniform Vec3f coneDir = section1.p - section0.p;
  uniform const float coneLength = length(coneDir);
  if (coneLength <= Epsilon)
     return false;
  const uniform coneLengthInv = rcp(coneLength);
  uniform const float AbsTan = abs(section1.r - section0.r)*coneLengthInv;
  uniform const float Tan2 = sqr(AbsTan);
  uniform bool cylinder = true;
  if (Tan2 > 10.f*Epsilon && abs(section0.r - section1.r) > 100.f*Epsilon && coneLength > 100.f*Epsilon)
      cylinder = false;
  uniform const float Cos2 = cylinder ? 1.f : rcp(1.f+Tan2);

  const Vec3f dir = make_Vec3f(ray.dirx,ray.diry,ray.dirz);
  const float rayLenInv = rcp_safe(length(dir));
  const Vec3f rayDir = dir*rayLenInv;
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
        tip = top + coneDir*r/(section0.r-section1.r);
        coneDir = neg(coneDir);
     } else {
        widening = true;
        r = section0.r;
        top = section0.p;
        tip = top + coneDir*r/(section0.r-section1.r);
     }
  }
  coneDir = normalize(coneDir);
  uniform const Vec3f M0 = make_Vec3f(coneDir.x*coneDir.x-Cos2, coneDir.x*coneDir.y, coneDir.x*coneDir.z);
  uniform const Vec3f M1 = make_Vec3f(coneDir.x*coneDir.y, coneDir.y*coneDir.y-Cos2, coneDir.y*coneDir.z);
  uniform const Vec3f M2 = make_Vec3f(coneDir.x*coneDir.z, coneDir.y*coneDir.z, coneDir.z*coneDir.z-Cos2);
  const Vec3f Mxdir = make_Vec3f(dot(M0, rayDir), dot(M1, rayDir), dot(M2, rayDir));
  const float c2 = dot(rayDir, Mxdir);
  if (abs(c2) < Epsilon) {
     // on cone boundary
     return false;
  }
  const Vec3f org = make_Vec3f(ray.orgx,ray.orgy,ray.orgz);
  const Vec3f Delta = org - tip;
  const Vec3f MxDelta = make_Vec3f(dot(M0, Delta), dot(M1, Delta), dot(M2, Delta));
  const float c1 = dot(rayDir, MxDelta);
  float c0 = dot(Delta, MxDelta);
  if (cylinder) {
      c0 += sqr(r);
  }
  const float disc = c1*c1 - c0*c2;
  if (disc < 0) {
     // no intersection
     return false;
  }

  const float sqrtDisc = sqrt(disc);
  const float c2Inv = rcp(c2);
  const float t0 = (-c1 + sqrtDisc)*c2Inv*rayLenInv;
  const float t1 = (-c1 - sqrtDisc)*c2Inv*rayLenInv;

  //print("|n=% f=% 0=% 1=%|.", ray.tnear, ray.tfar, t0, t1);

  bool hit = false;
  if ((ray.tnear < t0) & (t0 < ray.tfar)) {
    const Vec3f pRel = org+t0*dir - top;
    const float topDist = dot(coneDir, pRel);
    if (topDist >= 0 && topDist <= coneLength) {
       hit = true;
       const Vec3f pRelProj = coneDir * topDist;

       ray.tfar = t0;
       const Vec3f Ng = normalize(pRel - pRelProj) - coneDir*AbsTan;
       ray.Ngx = Ng.x;
       ray.Ngy = Ng.y;
       ray.Ngz = Ng.z;
       ray.u = widening ? topDist*coneLengthInv : 1.f - topDist*coneLengthInv;
       ray.v = cylinder ? 0.0f : 1.f-ray.u;
    }
  }
  if ((ray.tnear < t1) & (t1 < ray.tfar)) {
    const Vec3f pRel = org+t1*dir - top;
    const float topDist = dot(coneDir, pRel);
    if (topDist >= 0 && topDist <= coneLength) {
       hit = true;
       const Vec3f pRelProj = coneDir * topDist;

       ray.tfar = t1;
       const Vec3f Ng = normalize(pRel - pRelProj) - coneDir*AbsTan;
       ray.Ngx = Ng.x;
       ray.Ngy = Ng.y;
       ray.Ngz = Ng.z;
       ray.u = widening ? topDist*coneLengthInv : 1.f - topDist*coneLengthInv;
       ray.v = cylinder ? 0.0f : 1.f-ray.u;
    }
  }

  return hit;
}

bool discIsect(const uniform TubeSection &center,
      const uniform Vec3f &normal, varying RTCRay &ray) {

   const Vec3f dir = make_Vec3f(ray.dirx,ray.diry,ray.dirz);
   const float prod = dot(normal, dir);
   if (abs(prod) < Epsilon) {
      // ray and disc are parallel
      return false;
   }

   const Vec3f org = make_Vec3f(ray.orgx,ray.orgy,ray.orgz);
   const float t = dot(normal, center.p - org)/prod;
   if ((ray.tnear < t) & (t < ray.tfar)) {
      const Vec3f p = org+t*dir;
      if (length(center.p - p) <= center.r) {
         ray.u = 0.f;
         ray.v = 0.f;
         ray.tfar = t;
         const Vec3f Ng = normal;
         ray.Ngx = Ng.x;
         ray.Ngy = Ng.y;
         ray.Ngz = Ng.z;

         return true;
      }
   }
   return false;
}

struct Plane {
    Vec3f normal;
    float dist;
};

inline bool sphereSectorIsect(const uniform Sphere &sphere, const uniform Plane &plane0, const uniform Plane &plane1, varying RTCRay &ray) {

  const Vec3f dir = make_Vec3f(ray.dirx,ray.diry,ray.dirz);
  const Vec3f org = make_Vec3f(ray.orgx,ray.orgy,ray.orgz);
  const Vec3f v = org-sphere.p;
  const float A = dot(dir,dir);
  const float B = dot(v,dir);
  const float C = dot(v,v) - sqr(sphere.r);
  const float D = B*B - A*C;
  if (D < 0.0f) return false;
  const float Q = sqrt(D);
  const float rcpA = rcp(A);
  const float t0 = rcpA*(-B-Q);
  const float t1 = rcpA*(-B+Q);
  bool hit = false;
  float t = ray.tfar;
  if ((ray.tnear < t0) & (t0 < ray.tfar)) {
     hit = true;
     t = t0;
  }
  if ((ray.tnear < t1) & (t1 < t)) {
     hit = true;
     t = t1;
  }
  if (hit) {
     const Vec3f p = org+t*dir;
     if (dot(plane0.normal, p) < plane0.dist-Epsilon) {
         return false;
     }
     if (dot(plane1.normal, p) < plane1.dist-Epsilon) {
         return false;
     }
     ray.tfar = t;
     const Vec3f Ng = p-sphere.p;
     ray.Ngx = Ng.x;
     ray.Ngy = Ng.y;
     ray.Ngz = Ng.z;
     ray.u = 1.f; // at end of tube section
     ray.v = 0.f;
  }
  return hit;
}



// ======================================================================== //
//                     User defined tube geometry                         //
// ======================================================================== //

unmasked void tubeBoundsFunc(void *uniform ptr, uniform size_t item, uniform RTCBounds &bounds_o) {
   const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
   const uniform unsigned int flags = data->primitiveFlags[item];
   if (flags == PFNone) {
      return;
   }
   const uniform unsigned int Start = PFCone|PFStartSphere|PFStartDisc;
   const uniform unsigned int End = PFCone|PFEndSphere|PFEndSphereSect|PFEndDisc|PFArrow;

   const uniform TubeSection& section0 = data->spheres[item];
   const uniform float r1 = section0.r;
   const uniform TubeSection& section1 = data->spheres[item+1];
   const uniform float r2 = (flags&PFArrow) ? ArrowSize*section1.r : section1.r;

   uniform Vec3f lower, upper;
   if (flags & Start) {
      if (flags & End) {
         lower = min(section0.p-make_Vec3f(r1), section1.p-make_Vec3f(r2));
         upper = max(section0.p+make_Vec3f(r1), section1.p+make_Vec3f(r2));
      } else {
         lower = section0.p-make_Vec3f(r1);
         upper = section0.p+make_Vec3f(r1);
      }
   } else {
      lower = section1.p-make_Vec3f(r2);
      upper = section1.p+make_Vec3f(r2);
   }

   bounds_o.lower_x = lower.x;
   bounds_o.lower_y = lower.y;
   bounds_o.lower_z = lower.z;
   bounds_o.upper_x = upper.x;
   bounds_o.upper_y = upper.y;
   bounds_o.upper_z = upper.z;
}

void tubeIntersectFunc(void *uniform ptr, varying RTCRay& ray, uniform size_t item)
{
  const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
  const uniform TubeSection& section0 = data->spheres[item];
  const uniform TubeSection& section1 = data->spheres[item+1];
  const uniform unsigned int flags = data->primitiveFlags[item];

  if (flags & PFCone) {
     if (coneSectionIsect(section0, section1, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
     }
  }

  if (flags & PFStartSphere) {
     if (sphereIsect(section0, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
     }
  }
  if (flags & PFEndSphere) {
     if (sphereIsect(section1, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
     }
  }
  if (flags & PFEndSphereSect) {
     uniform Plane p0, p1;
     p0.normal = normalize(section1.p-section0.p);
     p0.dist = dot(p0.normal, section1.p);
     p1.normal = normalize(section1.p-data->spheres[item+2].p);
     p1.dist = dot(p1.normal, section1.p);
     if (sphereSectorIsect(section1, p0, p1, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
     }
  }

  if (flags & PFStartDisc) {
     uniform Vec3f coneDir = normalize(section1.p - section0.p);
     if (discIsect(section0, coneDir, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
     }
  }
  if (flags & PFEndDisc) {
     uniform Vec3f coneDir = normalize(section0.p - section1.p);
     if (discIsect(section1, coneDir, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
     }
  }
  if (flags & PFArrow) {
     uniform Vec3f coneDir = normalize(section1.p - section0.p);
     uniform TubeSection a1, a2;
     a1.p = section1.p;
     a1.r = ArrowSize * section1.r;
     a2.p = section1.p + coneDir * a1.r;
     a2.r = 0.f;
     if (coneSectionIsect(a1, a2, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
        ray.u = ray.v = 0.f;
     }
     if (discIsect(a1, coneDir, ray)) {
        ray.geomID = data->geomId;
        ray.primID = item;
        ray.u = ray.v = 0.f;
     }
  }
}

#if 0
void tubeIntersectFuncN(const uniform int* uniform valid, /*! pointer to valid mask */
                                           RenderObjectData *uniform data,                /*!< pointer to geometry user data */
                                           const uniform RTCIntersectContext* uniform context,  /*!< intersection context as passed to rtcIntersect/rtcOccluded */
                                           RTCRayN* uniform rays,               /*!< ray packet of size N */
                                           uniform size_t N,                 /*< number of rays in ray packet */
                                           uniform size_t item              /*< item to intersect */)
{
    const uniform TubeSection& section0 = data->spheres[item];
    const uniform TubeSection& section1 = data->spheres[item+1];
    const uniform unsigned int flags = data->primitiveFlags[item];

    varying RTCRay ray;
    /* iterate over all rays in ray packet */
    for (uniform unsigned int ui=0; ui<N; ui+=programCount)
    {
        /* calculate varying loop and execution mask */
        unsigned int vi = ui+programIndex;
        if (vi>=N) continue;

        /* ignore inactive rays */
        if (valid[vi] != -1) continue;

        ray.org.x = RTCRayN_org_x(rays, N, ui);

        if (flags & PFCone) {
            if (coneSectionIsect(section0, section1, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
            }
        }

        if (flags & PFStartSphere) {
            if (sphereIsect(section0, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
            }
        }
        if (flags & PFEndSphere) {
            if (sphereIsect(section1, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
            }
        }
        if (flags & PFEndSphereSect) {
            uniform Plane p0, p1;
            p0.normal = normalize(section1.p-section0.p);
            p0.dist = dot(p0.normal, section1.p);
            p1.normal = normalize(section1.p-data->spheres[item+2].p);
            p1.dist = dot(p1.normal, section1.p);
            if (sphereSectorIsect(section1, p0, p1, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
            }
        }

        if (flags & PFStartDisc) {
            uniform Vec3f coneDir = normalize(section1.p - section0.p);
            if (discIsect(section0, coneDir, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
            }
        }
        if (flags & PFEndDisc) {
            uniform Vec3f coneDir = normalize(section0.p - section1.p);
            if (discIsect(section1, coneDir, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
            }
        }
        if (flags & PFArrow) {
            uniform Vec3f coneDir = normalize(section1.p - section0.p);
            uniform TubeSection a1, a2;
            a1.p = section1.p;
            a1.r = ArrowSize * section1.r;
            a2.p = section1.p + coneDir * a1.r;
            a2.r = 0.f;
            if (coneSectionIsect(a1, a2, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
                ray.u = ray.v = 0.f;
            }
            if (discIsect(a1, coneDir, ray)) {
                ray.geomID = data->geomId;
                ray.primID = item;
                ray.u = ray.v = 0.f;
            }
        }
    }
}
#endif

void tubeOccludedFunc(void *uniform ptr, varying RTCRay& ray, uniform size_t item)
{
  const uniform RenderObjectData *uniform data = (const uniform RenderObjectData *uniform)ptr;
  const uniform TubeSection& section = data->spheres[item];
  const Vec3f dir = make_Vec3f(ray.dirx,ray.diry,ray.dirz);
  const Vec3f org = make_Vec3f(ray.orgx,ray.orgy,ray.orgz);
  const Vec3f v = org-section.p;
  const float A = dot(dir,dir);
  const float B = 2.0f*dot(v,dir);
  const float C = dot(v,v) - sqr(section.r);
  const float D = B*B - 4.0f*A*C;
  if (D < 0.0f) return;
  const float Q = sqrt(D);
  const float rcpA = rcp(A);
  const float t0 = 0.5f*rcpA*(-B-Q);
  const float t1 = 0.5f*rcpA*(-B+Q);
  if ((ray.tnear < t0) & (t0 < ray.tfar)) {
    ray.geomID = 0;
  }
  if ((ray.tnear < t1) & (t1 < ray.tfar)) {
    ray.geomID = 0;
  }
}

export uniform unsigned int registerTubes(uniform RTCScene scene, uniform RenderObjectData *uniform data, uniform size_t N) {

  assert(data->spheres);

  const uniform unsigned int geomID = rtcNewUserGeometry(scene,N);
  data->geomId = geomID;
  rtcSetUserData(scene, geomID, data);
  rtcSetBoundsFunction(scene, geomID, &tubeBoundsFunc);
#ifdef USE_STREAM
  //rtcSetIntersectFunctionN(scene, geomID, (RTCIntersectFuncN)&tubeIntersectFuncN);
#else
  rtcSetIntersectFunction(scene, geomID, &tubeIntersectFunc);
  rtcSetOccludedFunction(scene, geomID, &tubeOccludedFunc);
#endif
  return geomID;
}
