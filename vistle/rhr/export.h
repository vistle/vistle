#ifndef VISTLE_RHR_EXPORT_H
#define VISTLE_RHR_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_rhr_EXPORTS)
#define V_RHREXPORT V_EXPORT
#else
#define V_RHREXPORT V_IMPORT
#endif

#endif
