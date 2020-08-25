#ifndef VISIT_INSITU_UTIL_EXPORT_H
#define VISIT_INSITU_UTIL_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_insitu_core_EXPORTS)
#define V_INSITUCOREEXPORT V_EXPORT
#else
#define V_INSITUCOREEXPORT V_IMPORT
#endif

#endif
