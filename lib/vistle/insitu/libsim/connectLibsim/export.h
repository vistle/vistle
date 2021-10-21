#ifndef LIBSIM_ESTABLISH_CONNECTION_EXPORT_H
#define LIBSIM_ESTABLISH_CONNECTION_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_libsim_connect_EXPORTS)
#define V_LIBSIMCONNECTEXPORT V_EXPORT
#else
#define V_LIBSIMCONNECTEXPORT V_IMPORT
#endif

#endif
