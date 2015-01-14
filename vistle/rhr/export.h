#ifndef VISTLE_RHR_EXPORT_H
#define VISTLE_RHR_EXPORT_H

#include <util/export.h>

#if defined (vistle_rhr_EXPORTS)
#define V_RHREXPORT V_EXPORT
#else
#define V_RHREXPORT V_IMPORT
#endif

#if defined (vistle_rhr_EXPORTS)
#define V_RHRTEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_RHRTEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif

#endif
