 #ifndef _MASTER_H_
 #define _MASTER_H_
 #define DEFER_TRANSIENT_OPERATIONS
 #if defined TP_DIRECT               || \
 defined TP_ACQUIRES             || \
 defined TP_RELEASES             || \
 defined TP_OUT                  || \
 defined TP_IN_OUT               || \
 defined TP_ARRAY_OUT            || \
 defined TP_ARRAY_IN_OUT         || \
 defined TP_GIVES                || \
 defined TP_RECEIVES             || \
 defined TP_RECEIVES_GIVES       || \
 defined TP_ARRAY_GIVES          || \
 defined TP_ARRAY_RECEIVES       || \
 defined TP_ARRAY_RECEIVES_GIVES || \
 defined TP_QUERY
 #error "Tecplot's attribute keywords are in direct conflict with other meanings."
 #endif
 #if defined USE_GCCXML_ATTRIBUTES
 #define TP_DIRECT               __attribute((___1544("direct")))
 #define TP_ACQUIRES             __attribute((___1544("acquires","in")))
 #define TP_RELEASES             __attribute((___1544("releases","in")))
 #define TP_OUT                  __attribute((___1544("out")))
 #define TP_IN_OUT               __attribute((___1544("in","out")))
 #define TP_ARRAY_OUT            __attribute((___1544("array","out")))
 #define TP_ARRAY_IN_OUT         __attribute((___1544("array","in","out")))
 #define TP_GIVES                __attribute((___1544("gives","out")))
 #define TP_RECEIVES             __attribute((___1544("receives","in")))
 #define TP_RECEIVES_GIVES       __attribute((___1544("receives","in","gives","out")))
 #define TP_ARRAY_GIVES          __attribute((___1544("array","gives","out")))
 #define TP_ARRAY_RECEIVES       __attribute((___1544("array","receives","in")))
 #define TP_ARRAY_RECEIVES_GIVES __attribute((___1544("array","receives","in","gives","out")))
 #define TP_QUERY                __attribute((___1544("query")))
 #else
 #define TP_DIRECT
 #define TP_ACQUIRES
 #define TP_RELEASES
 #define TP_OUT
 #define TP_IN_OUT
 #define TP_ARRAY_OUT
 #define TP_ARRAY_IN_OUT
 #define TP_GIVES
 #define TP_RECEIVES
 #define TP_RECEIVES_GIVES
 #define TP_ARRAY_GIVES
 #define TP_ARRAY_RECEIVES
 #define TP_ARRAY_RECEIVES_GIVES
 #define TP_QUERY
 #endif
#include "stdafx.h"
#include <string>
#include <map>
#include <vector>
#include <queue>
 #if defined _WIN32
 #if !defined TECPLOTKERNEL
 #if !defined MSWIN
 #define MSWIN
 #endif 
 #if !defined WINDOWS
 #define WINDOWS
 #endif 
 #if !defined _WINDOWS
 #define _WINDOWS
 #endif 
 #if !defined WIN32
 #define WIN32
 #endif 
 #if defined _DEBUG
 #if !defined DEBUG
 #define DEBUG
 #endif
 #elif defined CHECKED_BUILD
 #if defined NO_ASSERTS
 #undef NO_ASSERTS
 #endif
 #if defined NDEBUG
 #undef NDEBUG 
 #endif
 #else 
 #if !defined NDEBUG
 #define NDEBUG
 #endif
 #endif 
 #endif 
 #if _MSC_VER >= 1400
 #define ___4441 
 #endif
 #if !defined TECPLOTKERNEL && defined ___4441
 #if !defined _CRT_SECURE_NO_DEPRECATE
 #define _CRT_SECURE_NO_DEPRECATE
 #endif
 #endif 
 #endif 
 #ifdef NDEBUG
 # ifdef _DEBUG
 #   error "Both NDEBUG and _DEBUG defined"
 # endif
 #elif defined TECPLOTKERNEL
 # ifndef _DEBUG
 #   define _DEBUG
 # endif
 #endif
 #if !defined TECPLOTKERNEL
 #if defined NDEBUG && !defined CHECKED_BUILD && !defined NO_ASSERTS
 #define NO_ASSERTS
 #endif
 #endif
 #ifdef NO_ASSERTS 
 #define ___3585 ___1527
 #define ___3231
 #endif 
#include "TranslatedString.h"
 #define ___4279
 #ifndef THREED
 #  define THREED
 #endif
#include <stdio.h>
#include <ctype.h>
#include <math.h>
 #if defined ___3258
 #define ___959
 #endif
 #if defined ___2465
 #define ___1098
 #endif
 #if defined CRAYX
 #define CRAY
 #endif
 #if defined ___1993
 #define ___1992
 #endif
 #if defined HPX
 #define HPUX
 #define ___1829
 #endif
 #if defined IBMRS6000X
 #define ___1831
 #endif
 #if defined COMPAQALPHAX
 #define ___532
 #define COMPAQX
 #define COMPAQ
 #endif
 #if defined DECALPHAX
 #define DECALPHA
 #define DECX
 #endif
 #if defined DECX
 #define DEC
 #endif
 #if defined ___3890 || defined ___3889
 #define ___3891
 #endif
 #if defined ___3891
 #define ___3884
 #endif
 #if defined ___1993 || defined CRAYX || defined HPX || defined ___3891 || defined ___655
 #define UNIXX
 #define ___3920
 #endif
 #if defined DECX || defined LINUX || defined IBMRS6000X || defined COMPAQX || defined DARWIN
 #define UNIXX
 #endif
#include <stdarg.h>
 #define OEM_INVALID_CHECKSUM (___2225) -1
 #if defined MSWIN
 #define USE_TRUETYPEFONTS
 #endif
 #ifdef MSWIN
 #if defined ___4441
 #define Widget ___2320 
 #else
 #define Widget long
 #endif
 #endif 
 #if defined UNIXX
typedef void *Widget;
 #endif
#include <string.h>
 #if !defined ___3920 && !defined MSWIN
#include <strings.h>
 #endif
 #if defined (___2465)
#include <stdlib.h>
 #define ___1197
 #ifndef ___1304
 #define ___1304
 #endif
 #define VOID       void
 #endif
#include <sys/types.h>
#include <stdlib.h>
 #if defined UNIXX
 #define ___1304
 #define ___2688
#include <unistd.h>
 #endif
 #if defined MSWIN
#include <windows.h>
 #endif
 #if !defined (TRACE)
 #if defined NDEBUG
 #if defined MSWIN
 #define TRACE                       __noop
 #define TRACE0(s)                   __noop
 #define TRACE1(S,a1)                __noop
 #define TRACE2(s,a1,a2)             __noop
 #define TRACE3(s,a1,a2,a3)          __noop
 #define TRACE4(s,a1,a2,a3,a4)       __noop
 #define TRACE5(s,a1,a2,a3,a4,a5)    __noop
 #define TRACE6(s,a1,a2,a3,a4,a5,a6) __noop
 #else
 #define TRACE(str)                      ((void)0)
 #define TRACE0(str)                     ((void)0)
 #define TRACE1(str,a1)                  ((void)0)
 #define TRACE2(str,a1,a2)               ((void)0)
 #define TRACE3(str,a1,a2,a3)            ((void)0)
 #define TRACE4(str,a1,a2,a3,a4)         ((void)0)
 #define TRACE5(str,a1,a2,a3,a4,a5)      ((void)0)
 #define TRACE6(str,a1,a2,a3,a4,a5,a6)   ((void)0)
 #endif 
 #else 
 #if defined MSWIN
 # define TRACE(str)                    do {                                                 OutputDebugStringA(str); }    while (0)
 # define TRACE1(str,a1)                do { char s[5000]; sprintf(s,str,a1);                OutputDebugStringA(s);   }    while (0)
 # define TRACE2(str,a1,a2)             do { char s[5000]; sprintf(s,str,a1,a2);             OutputDebugStringA(s);   }    while (0)
 # define TRACE3(str,a1,a2,a3)          do { char s[5000]; sprintf(s,str,a1,a2,a3);          OutputDebugStringA(s);   }    while (0)
 # define TRACE4(str,a1,a2,a3,a4)       do { char s[5000]; sprintf(s,str,a1,a2,a3,a4);       OutputDebugStringA(s);   }    while (0)
 # define TRACE5(str,a1,a2,a3,a4,a5)    do { char s[5000]; sprintf(s,str,a1,a2,a3,a4,a5);    OutputDebugStringA(s);   }    while (0)
 # define TRACE6(str,a1,a2,a3,a4,a5,a6) do { char s[5000]; sprintf(s,str,a1,a2,a3,a4,a5,a6); OutputDebugStringA(s);   }    while (0)
 # define TRACE0(str) TRACE(str)
 #else
 #define TRACE  printf
 #define TRACE0 printf
 #define TRACE1 printf
 #define TRACE2 printf
 #define TRACE3 printf
 #define TRACE4 printf
 #define TRACE5 printf
 #define TRACE6 printf
 #endif 
 #endif 
 #endif 
 #if !defined MAX_SIZEOFUTF8CHAR
 #define MAX_SIZEOFUTF8CHAR 1
 #endif
 #if !defined (MaxCharsFilePath)
 # if defined (MSWIN)
 #   define MaxCharsFilePath (_MAX_PATH*MAX_SIZEOFUTF8CHAR+1) 
 # else
 #   define MaxCharsFilePath 2047 
 # endif 
 #endif 
 #if defined MSWIN && defined NDEBUG && !defined NO_ASSERTS && !defined CHECKED_BUILD
 #  error "define NO_ASSERTS for release builds"
 #endif
 #if defined NO_ASSERTS
 #  if !defined USE_MACROS_FOR_FUNCTIONS
 #    define USE_MACROS_FOR_FUNCTIONS
 #  endif
 #endif
 #if defined LINUX && defined NULL
 # undef NULL
 # define NULL 0
 #endif
 #if defined MSWIN || defined LINUX || defined DARWIN
 #define ___1821
 #endif
 #if defined __GNUC__ && !defined ___1543
 #define ___1543 (__GNUC__ * 10000 + \
 __GNUC_MINOR__ * 100 + \
 __GNUC_PATCHLEVEL__)
 #endif
 #if defined MSWIN && defined max
 # undef max
 #endif
 #if defined MSWIN && defined min
 # undef min
 #endif
 #endif 
