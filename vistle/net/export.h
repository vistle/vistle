#ifndef VISTLE_NET_EXPORT_H
#define VISTLE_NET_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_net_EXPORTS)
#define V_NETEXPORT V_EXPORT
#else
#define V_NETEXPORT V_IMPORT
#endif

#endif
