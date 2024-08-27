#ifndef VISTLE_ADAPTER_EXPORT_H
#define VISTLE_ADAPTER_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_insitu_adapter_EXPORTS)
#define V_INSITUADAPTEREXPORT V_EXPORT
#else
#define V_INSITUADAPTEREXPORT V_IMPORT
#endif

#if defined(vistle_insitu_adapter_EXPORTS)
#define V_INSITUADAPTER_TEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_INSITUADAPTER_TEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif

#endif
