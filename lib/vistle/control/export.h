#ifndef VISTLE_HUB_EXPORT_H
#define VISTLE_HUB_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_control_EXPORTS)
#define V_HUBEXPORT V_EXPORT
#else
#define V_HUBEXPORT V_IMPORT
#endif

#endif
