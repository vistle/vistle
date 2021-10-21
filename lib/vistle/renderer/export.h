#ifndef VISTLE_RENDERER_EXPORT_H
#define VISTLE_RENDERER_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_renderer_EXPORTS)
#define V_RENDEREREXPORT V_EXPORT
#else
#define V_RENDEREREXPORT V_IMPORT
#endif

#endif
