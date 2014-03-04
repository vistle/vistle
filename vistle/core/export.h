#ifndef VISTLE_EXPORT_H
#define VISTLE_EXPORT_H

#include <util/export.h>

#if defined (vistle_core_EXPORTS)
#define V_COREEXPORT V_EXPORT
#else
#define V_COREEXPORT V_IMPORT
#endif

#if defined (vistle_core_EXPORTS)
#define V_CORETEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_CORETEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif

#endif
