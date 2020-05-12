#ifndef VISIT_INSITU_UTIL_EXPORT_H
#define VISIT_INSITU_UTIL_EXPORT_H

#include <util/export.h>


#if defined(vistle_insitu_core_EXPORTS)
#define V_INSITUCOREEXPORT V_EXPORT
#else
#define V_INSITUCOREEXPORT V_IMPORT
#endif

#if defined (vistle_insitu_core_EXPORTS)
#define V_INSITU_CORETEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_INSITU_CORETEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif


#endif