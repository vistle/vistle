#ifndef VISTLE_CORE_EXPORT_H
#define VISTLE_CORE_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_core_EXPORTS)
#define V_COREEXPORT V_EXPORT
#else
#define V_COREEXPORT V_IMPORT
#endif

#endif
