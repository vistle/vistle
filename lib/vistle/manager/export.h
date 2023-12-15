#ifndef VISTLE_MANAGER_EXPORT_H
#define VISTLE_MANAGER_EXPORT_H

#ifdef MODULE_THREAD
#include <vistle/util/export.h>

#if defined(vistle_clustermanager_EXPORTS)
#define V_MANAGEREXPORT V_EXPORT
#else
#define V_MANAGEREXPORT V_IMPORT
#endif
#else
#define V_MANAGEREXPORT
#endif // MODULE_THREAD


#endif
