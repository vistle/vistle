#ifndef VISIT_RUNTIME_EXPORT_H
#define VISIT_RUNTIME_EXPORT_H

#include <vistle/util/export.h>

#if defined(LIBSIM_EXPORT)
#define V_VISITXPORT V_EXPORT
#else
#define V_VISITXPORT V_IMPORT
#endif

#endif
