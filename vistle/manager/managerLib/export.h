#ifndef VISTLE_MANAGER_EXPORT_H
#define VISTLE_MANAGER_EXPORT_H

#include <util/export.h>

#if defined (vistle_manager_lib_EXPORTS)
#define V_MANAGEREXPORT V_EXPORT
#else
#define V_MANAGEREXPORT V_IMPORT
#endif

#endif
