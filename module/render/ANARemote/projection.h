/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#pragma once

#include <vistle/core/vector.h>

namespace glm {

using vec2 = vistle::Vector2;
using vec3 = vistle::Vector3;
using vec4 = vistle::Vector4;
using mat3 = vistle::Matrix3;
using mat4 = vistle::Matrix4;


struct box2 {
    vec2 min, max;
};

void transformDepthFromWorldToGL(const float *world, float *gl, vec3 eye, vec3 dir, vec3 up, float fovy, float aspect,
                                 box2 imageRegion, mat4 view, mat4 proj, int width, int heigh);

// inlineable functions //

static void offaxisStereoCamera(vec3 LL, vec3 LR, vec3 UR, vec3 eye, vec3 &dirOUT, vec3 &upOUT, float &fovyOUT,
                                float &aspectOUT, box2 &imageRegionOUT)
{
    vec3 X = (LR - LL) / (LR - LL).norm();
    vec3 Y = (UR - LR) / (UR - LR).norm();
    vec3 Z = X.cross(Y);

    dirOUT = -Z;
    upOUT = Y;

    // eye position relative to screen/wall
    vec3 eyeP = eye - LL;

    // distance from eye to screen/wall
    float dist = eyeP.dot(Z);

    float left = eyeP.dot(X);
    float right = (LR - LL).norm() - left;
    float bottom = eyeP.dot(Y);
    float top = (UR - LR).norm() - bottom;

    float newWidth = left < right ? 2 * right : 2 * left;
    float newHeight = bottom < top ? 2 * top : 2 * bottom;

    fovyOUT = 2 * atan(newHeight / (2 * dist));

    aspectOUT = newWidth / newHeight;

    imageRegionOUT.min[0] = left < right ? (right - left) / newWidth : 0.f;
    imageRegionOUT.max[0] = right < left ? (left + right) / newWidth : 1.f;
    imageRegionOUT.min[1] = bottom < top ? (top - bottom) / newHeight : 0.f;
    imageRegionOUT.max[1] = top < bottom ? (bottom + top) / newHeight : 1.f;
}

static vec3 unprojectNDC(mat4 projInv, mat4 viewInv, vec3 ndc)
{
    vec4 ndc4;
    ndc4 << ndc, 1;
    vec4 v = viewInv * (projInv * ndc4);
    return vec3(v[0], v[1], v[2]) / v[3];
}

static bool intersectPlanePlane(vec3 na, vec3 pa, vec3 nb, vec3 pb, vec3 &nl, vec3 &pl)
{
    vec3 nc = na.cross(nb);
    float det = nc.squaredNorm();

    if (det == 0.f)
        return false;

    float da = -na.dot(pa);
    float db = -nb.dot(pb);

    pl = ((nc.cross(nb) * da) + (na.cross(nc) * db)) / det;
    nl = nc;

    return true;
}

static bool solve(mat3 A, vec3 b, vec3 &x)
{
    float D = A.determinant();
    if (D == 0.f)
        return false;

    mat3 M1, M2, M3;
    M1 << b, A.col(1), A.col(2);
    M2 << A.col(0), b, A.col(2);
    M3 << A.col(0), A.col(1), b;

    x = vec3(M1.determinant(), M2.determinant(), M3.determinant()) / D;

    return true;
}

static void closestLineSegmentBetweenTwoLines(vec3 na, vec3 pa, vec3 nb, vec3 pb, vec3 &pc1, vec3 &pc2)
{
    vec3 nc = na.cross(nb).normalized();
    vec3 b = pb - pa;
    mat3 A;
    A << na, -nb, nc;
    vec3 x;
    if (!solve(A, b, x))
        return;
    pc1 = pa + na * x[0];
    pc2 = pb + nb * x[1];
}

static void offaxisStereoCameraFromTransform(mat4 projInv, mat4 viewInv, vec3 &eyeOUT, vec3 &dirOUT, vec3 &upOUT,
                                             float &fovyOUT, float &aspectOUT, box2 &imageRegionOUT)
{
    // Transform NDC unit cube corners to world/CAVE space
    vec3 v000 = unprojectNDC(projInv, viewInv, vec3(-1, -1, -1));
    vec3 v001 = unprojectNDC(projInv, viewInv, vec3(-1, -1, 1));

    vec3 v100 = unprojectNDC(projInv, viewInv, vec3(1, -1, -1));
    vec3 v101 = unprojectNDC(projInv, viewInv, vec3(1, -1, 1));

    vec3 v110 = unprojectNDC(projInv, viewInv, vec3(1, 1, -1));
    //vec3 v111 = unprojectNDC(projInv, viewInv, vec3(1, 1, 1));

    vec3 v010 = unprojectNDC(projInv, viewInv, vec3(-1, 1, -1));
    vec3 v011 = unprojectNDC(projInv, viewInv, vec3(-1, 1, 1));

    // edges from -z to +z
    vec3 ez00 = (v001 - v000).normalized();
    vec3 ez10 = (v101 - v100).normalized();
    //vec3 ez11 = (v111 - v110).normalized();
    vec3 ez01 = (v011 - v010).normalized();

    // edges from -y to +y
    vec3 ey00 = (v010 - v000).normalized();
    vec3 ey10 = (v110 - v100).normalized();
    //vec3 ey11 = (v111 - v101).normalized();
    //vec3 ey01 = (v011 - v001).normalized();

    // edges from -x to +x
    vec3 ex00 = (v100 - v000).normalized();
    vec3 ex10 = (v110 - v010).normalized();
    //vec3 ex11 = (v111 - v011).normalized();
    //vec3 ex01 = (v101 - v001).normalized();

    vec3 nL = (ey00.cross(ez00)).normalized();
    vec3 nR = (ez10.cross(ey10)).normalized();
    vec3 nB = (ez00.cross(ex00)).normalized();
    vec3 nT = (ex10.cross(ez01)).normalized();

    // Line of intersection between left/right planes
    vec3 pLR, nLR;
    intersectPlanePlane(nL, v000, nR, v100, nLR, pLR);

    // Line of intersection between bottom/top planes
    vec3 pBT, nBT;
    intersectPlanePlane(nB, v000, nT, v010, nBT, pBT);

    // Line segment connecting the two intersecint lines
    vec3 p1, p2;
    closestLineSegmentBetweenTwoLines(nLR, pLR, nBT, pBT, p1, p2);

    eyeOUT = (p1 + p2) / 2.f;

    vec3 LL = unprojectNDC(projInv, viewInv, vec3(-1, -1, 1));
    vec3 LR = unprojectNDC(projInv, viewInv, vec3(1, -1, 1));
    vec3 UR = unprojectNDC(projInv, viewInv, vec3(1, 1, 1));

    offaxisStereoCamera(LL, LR, UR, eyeOUT, dirOUT, upOUT, fovyOUT, aspectOUT, imageRegionOUT);
}

} // namespace glm
