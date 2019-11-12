#ifndef VISTLE_MANAGER_EXPORT_H
#define VISTLE_MANAGER_EXPORT_H

#include <util/export.h>

#if defined (run_on_main_thread_EXPORTS)
#define V_MAINTHREADEXPORT V_EXPORT
#else
#define V_MAINTHREADEXPORT V_IMPORT
#endif

#endif
