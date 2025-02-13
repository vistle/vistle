#ifndef VISTLE_INSITU_LIBSIM_EXPORT_H
#define VISTLE_INSITU_LIBSIM_EXPORT_H

#include <vistle/util/export.h>

#if defined(LIBSIM_EXPORT)
#define V_VISITXPORT V_EXPORT
#else
#define V_VISITXPORT V_IMPORT
#endif

#endif
