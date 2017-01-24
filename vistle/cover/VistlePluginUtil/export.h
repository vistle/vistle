#ifndef VISTLE_PLUGIN_UTIL_EXPORT_H
#define VISTLE_PLUGIN_UTIL_EXPORT_H

#include <util/export.h>

#if defined (VistlePluginUtil_EXPORTS)
#define V_PLUGINUTILEXPORT V_EXPORT
#else
#define V_PLUGINUTILEXPORT V_IMPORT
#endif

#if defined (VistlePluginUtil_EXPORTS)
#define V_PLUGINUTILTEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_PLUGINUTILTEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif

#endif
