#ifndef VISTLE_H
#define VISTLE_H


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

#if defined (vistle_core_EXPORTS)
#define VCEXPORT VEXPORT
#else
#define VCEXPORT VIMPORT
#endif


#ifdef _WIN32
typedef __int64 ssize_t;
typedef unsigned int uint;
#endif

#endif
