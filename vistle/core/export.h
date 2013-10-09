#ifndef VISTLE_EXPORT_H
#define VISTLE_EXPORT_H

#include <util/export.h>

#if defined (vistle_core_EXPORTS)
#define V_COREEXPORT V_EXPORT
#else
#define V_COREEXPORT V_IMPORT
#endif

#if defined (vistle_module_EXPORTS)
#define V_MODULEEXPORT V_EXPORT
#else
#define V_MODULEEXPORT V_IMPORT
#endif

#endif
