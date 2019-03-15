#ifndef VISTLE_HUB_EXPORT_H
#define VISTLE_HUB_EXPORT_H

#include <util/export.h>

#if defined (vistle_hub_EXPORTS)
#define V_HUBEXPORT V_EXPORT
#else
#define V_HUBEXPORT V_IMPORT
#endif

#endif

