/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <iostream>
#include <cmath>
#include "projection.h"
#include <vistle/util/math.h>

namespace glm {

void transformDepthFromWorldToGL(const float *world, float *gl, vec3 eye, vec3 dir, vec3 up, float fovy, float aspect,
                                 box2 imageRegion, mat4 view, mat4 proj, int width, int height)
{
    vec2 imgPlaneSize;
    imgPlaneSize[1] = 2.f * std::tan(0.5f * fovy);
    imgPlaneSize[0] = imgPlaneSize[1] * aspect;

    vec3 dir_du = dir.cross(up).normalized() * imgPlaneSize[0];
    vec3 dir_dv = dir_du.cross(dir).normalized() * imgPlaneSize[1];
    vec3 dir_00 = dir - .5f * dir_du - .5f * dir_dv;

    auto pv = proj * view;

    for (int y = 0; y < height; ++y) {
        float screen_y = vistle::lerp(imageRegion.min[1], imageRegion.max[1], y / (float)height);
        for (int x = 0; x < width; ++x) {
            size_t index = x + size_t(width) * y;
            float t = world[index];
            if (std::isfinite(t)) {
                float screen_x = vistle::lerp(imageRegion.min[0], imageRegion.max[0], x / (float)width);

                vec3 ray_org = eye;
                vec3 ray_dir = (dir_00 + screen_x * dir_du + screen_y * dir_dv).normalized();

                vec4 worldPos4;
                worldPos4 << ray_org + ray_dir * t, 1;

                vec4 P4 = pv * worldPos4;
                vec3 glPos = vec3(P4[0], P4[1], P4[2]) / P4[3];

                gl[index] = vistle::clamp<float>((glPos[2] + 1.f) * .5f, 0.f, 1.f);
            } else {
                gl[index] = 1.f;
            }
        }
    }
}

} // namespace glm
