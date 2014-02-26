#ifndef RENDER_H
#define RENDER_H

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

static const RTCAlgorithmFlags intersections = RTC_INTERSECT1|RTC_INTERSECT4|RTC_INTERSECT8|RTC_INTERSECT16;

#define int8 char

#include "render-generic.h"

struct Vertex   { float x,y,z,align; };

#endif
