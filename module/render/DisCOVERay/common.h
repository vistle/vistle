#ifndef RAY_COMMON_H
#define RAY_COMMON_H

// clang-format off
#define IDENT(x) x
#define XSTR(x) #x
#define STR(x) XSTR(x)
#define EMBREE(y) STR(IDENT(embree)IDENT(EMBREE_MAJOR)IDENT(/)IDENT(y))
// clang-format on

#define TILESIZE 256
static const int TileSize = TILESIZE;

static const unsigned int RayEnabled = 0xffffffff;
static const unsigned int RayDisabled = 0;

#endif
