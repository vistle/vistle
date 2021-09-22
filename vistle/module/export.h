#ifndef VISTLE_MODULE_EXPORT_H
#define VISTLE_MODULE_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_module_EXPORTS)
#define V_MODULEEXPORT V_EXPORT
#else
#define V_MODULEEXPORT V_IMPORT
#endif

#endif
