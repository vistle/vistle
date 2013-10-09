#ifndef UTIL_EXPORT_H
#define UTIL_EXPORT_H

#if defined (_WIN32) && !defined (NODLL)
#define V_IMPORT __declspec(dllimport)
#define V_EXPORT __declspec(dllexport)

#elif defined(__GNUC__) && __GNUC__ >= 4
#define V_EXPORT __attribute__ ((visibility("default")))
#define V_IMPORT V_EXPORT
#else
#define V_IMPORT
#define V_EXPORT
#endif

#if defined (vistle_util_EXPORTS)
#define V_UTILEXPORT V_EXPORT
#else
#define V_UTILEXPORT V_IMPORT
#endif

#if defined (TEMPLATES_IN_HEADERS)
#define V_TEMPLATE_IMPORT
#define V_TEMPLATE_EXPORT
#else
#define V_TEMPLATE_IMPORT V_EXPORT
#define V_TEMPLATE_EXPORT V_IMPORT
#endif

#endif
