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
#include "linalg/vec3f.isph"
#include "linalg/affinespace.isph"

#include <embree2/rtcore.isph>
//#include <embree2/rtcore_ray.isph>
#include "rtcray.isph"
#include "render.isph"
#include "tubes.isph"

// ======================================================================== //
//                     User defined tube geometry                         //
// ======================================================================== //

void tubeBoundsFunc(const uniform RenderObjectData* uniform data, uniform size_t item, uniform RTCBounds* uniform bounds_o)
{
  const uniform TubeSection& section1 = data->spheres[item];
  const uniform TubeSection& section2 = data->spheres[item+1];

  if (data->primitiveFlags[item] & PFEnd) {
     return;
  }

  bounds_o->lower_x = min(section1.p.x-section1.r, section2.p.x-section2.r);
  bounds_o->lower_y = min(section1.p.y-section1.r, section2.p.y-section2.r);
  bounds_o->lower_z = min(section1.p.z-section1.r, section2.p.z-section2.r);
  bounds_o->upper_x = max(section1.p.x+section1.r, section2.p.x+section2.r);
  bounds_o->upper_y = max(section1.p.y+section1.r, section2.p.y+section2.r);
  bounds_o->upper_z = max(section1.p.z+section1.r, section2.p.z+section2.r);
}

void tubeIntersectFunc(const uniform RenderObjectData* uniform data, varying RTCRay& ray, uniform size_t item)
{
  if (data->primitiveFlags[item] & PFEnd) {
     return;
  }

  const uniform TubeSection& section1 = data->spheres[item];
  const uniform TubeSection& section2 = data->spheres[item+1];
  uniform Vec3f coneDir = section2.p - section1.p;
  uniform const float dist = length(coneDir);
  if (dist <= 1e-6)
     return;
  //coneDir = coneDir * rcp(dist);
  coneDir = normalize(coneDir);
  uniform const float Sin = (section2.r - section1.r)/dist;
  uniform const float Cos = sqrt(1-Sin*Sin);
  uniform const float r = section1.r;
  uniform const Vec3f V = section1.p;
  uniform const Vec3f M0 = make_Vec3f(coneDir.x*coneDir.x-Cos*Cos, coneDir.x*coneDir.y,   coneDir.x*coneDir.z);
  uniform const Vec3f M1 = make_Vec3f(coneDir.x*coneDir.y, coneDir.y*coneDir.y-Cos*Cos, coneDir.y*coneDir.z);
  uniform const Vec3f M2 = make_Vec3f(coneDir.x*coneDir.z,     coneDir.y*coneDir.z, coneDir.z*coneDir.z-Cos*Cos);

  varying const float rlen = length(ray.dir);
  varying const Vec3f rdir = normalize(ray.dir);
  varying const Vec3f Delta = ray.org - V;
  varying const Vec3f MxDelta = make_Vec3f(dot(M0, Delta), dot(M1, Delta), dot(M2, Delta));
  varying const Vec3f Mxdir = make_Vec3f(dot(M0, rdir), dot(M1, rdir), dot(M2, rdir));
  varying const float c2 = dot(rdir, Mxdir);
  varying const float c1 = dot(rdir, MxDelta);
  varying const float c0 = dot(Delta, MxDelta)+r*r;
  if (abs(c2) < 1e-6) {
     // on cone boundary
     return;
  }
  varying const float disc = c1*c1 - c0*c2;
  if (disc < 0) {
     // no intersection
     //print("-");
     return;
  }

  varying const float sqrtDisc = sqrt(disc);
  varying const float t0 = (-c1 + sqrtDisc)/c2/rlen;
  varying const float t1 = (-c1 - sqrtDisc)/c2/rlen;

  //print("|n=% f=% 0=% 1=%|.", ray.tnear, ray.tfar, t0, t1);

  if ((ray.tnear < t0) & (t0 < ray.tfar)) {
    varying const Vec3f pRel = ray.org+t0*ray.dir - V;
    varying const float tipDist = dot(coneDir, pRel);
    if (tipDist >= 0 && tipDist <= dist) {
       varying const Vec3f pRelProj = coneDir * tipDist;

       ray.u = tipDist/dist;
       ray.v = 0.0f;
       ray.tfar = t0;
       ray.geomID = data->geomId;
       ray.primID = item;
       ray.Ng = pRel - pRelProj;
    }
  }
  if ((ray.tnear < t1) & (t1 < ray.tfar)) {
    varying const Vec3f pRel = ray.org+t1*ray.dir - V;
    varying const float tipDist = dot(coneDir, pRel);
    if (tipDist >= 0 && tipDist <= dist) {
       varying const Vec3f pRelProj = coneDir * tipDist;

       ray.u = tipDist/dist;
       ray.v = 0.0f;
       ray.tfar = t1;
       ray.geomID = data->geomId;
       ray.primID = item;
       ray.Ng = pRel - pRelProj;
    }
  }
}

void tubeOccludedFunc(const uniform RenderObjectData* uniform data, varying RTCRay& ray, uniform size_t item)
{
  const uniform TubeSection& section = data->spheres[item];
  const Vec3f v = ray.org-section.p;
  const float A = dot(ray.dir,ray.dir);
  const float B = 2.0f*dot(v,ray.dir);
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
  rtcSetBoundsFunction(scene, geomID, (RTCBoundsFunc)&tubeBoundsFunc);
  rtcSetIntersectFunction(scene, geomID, (RTCIntersectFuncVarying)&tubeIntersectFunc);
  rtcSetOccludedFunction(scene, geomID, (RTCOccludedFuncVarying)&tubeOccludedFunc);
  return geomID;
}
