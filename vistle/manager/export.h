#ifndef RUN_ON_MAIN_THREAD_EXPORT_H
#define RUN_ON_MAIN_THREAD_EXPORT_H

#include <util/export.h>

#if defined (run_on_main_thread_EXPORTS)
#define V_MAINTHREADEXPORT V_EXPORT
#else
#define V_MAINTHREADEXPORT V_IMPORT
#endif

#endif
