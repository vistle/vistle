#ifndef VISTLE_COVER_EXPORT_H
#define VISTLE_COVER_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_cover_EXPORTS)
#define V_COVEREXPORT V_EXPORT
#else
#define V_COVEREXPORT V_IMPORT
#endif

#endif
