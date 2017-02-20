#ifndef UTIL_SSIZE_T_H
#define UTIL_SSIZE_T_H

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#endif
