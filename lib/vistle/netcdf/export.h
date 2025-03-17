#ifndef VISTLE_NETCDF_EXPORT_H
#define VISTLE_NETCDF_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_netcdf_EXPORTS)
#define V_NETCDFEXPORT V_EXPORT
#else
#define V_NETCDFEXPORT V_IMPORT
#endif

#endif
