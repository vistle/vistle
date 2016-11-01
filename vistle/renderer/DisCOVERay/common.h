#ifndef RAY_COMMON_H
#define RAY_COMMON_H

// Embree does not yet work with user defined geometry consisting of multiple items
//#define USE_STREAM // use Embree stream interface

static const int TileSize = 64;
#define TILESIZE 64
static const RTCSceneFlags sceneFlags = RTC_SCENE_COHERENT;

static const unsigned int RayEnabled = 0xffffffff;
static const unsigned int RayDisabled = 0;

#endif
