#ifndef VISTLE_ALG_EXPORT_H
#define VISTLE_ALG_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_alg_EXPORTS)
#define V_ALGEXPORT V_EXPORT
#else
#define V_ALGEXPORT V_IMPORT
#endif

#endif
