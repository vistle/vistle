#ifndef UTIL_EXPORT_H
#define UTIL_EXPORT_H

#if defined (_WIN32) && !defined (NODLL)
#define VIMPORT __declspec(dllimport)
#define VEXPORT __declspec(dllexport)

#elif defined(__GNUC__) && __GNUC__ >= 4
#define VEXPORT __attribute__ ((visibility("default")))
#define VIMPORT VEXPORT
#else
#define VIMPORT
#define VEXPORT
#endif

#if defined (vistle_util_EXPORTS)
#define UTILEXPORT VEXPORT
#else
#define UTILEXPORT VIMPORT
#endif

#endif
