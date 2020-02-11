#ifndef VISIT_RUNTIME_EXPORT_H
#define VISIT_RUNTIME_EXPORT_H

#include <util/export.h>


#if defined (LIBSIM_EXPORT)
#define V_VISITXPORT V_EXPORT
#else
#define V_VISITXPORT V_IMPORT
#endif

#if defined (TEMPLATES_IN_HEADERS)
#define V_TEMPLATE_IMPORT
#define V_TEMPLATE_EXPORT
#else
#define V_TEMPLATE_IMPORT V_EXPORT
#define V_TEMPLATE_EXPORT V_IMPORT
#endif

#endif