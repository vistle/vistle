#ifndef VISTLE_UTIL_SYSDEP_H
#define VISTLE_UTIL_SYSDEP_H

#include "export.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <cstdio>
#include <process.h>
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint32_t uid_t;

uid_t V_UTILEXPORT getuid();

void V_UTILEXPORT sleep(int x);
void V_UTILEXPORT usleep(__int64 usec);
int V_UTILEXPORT setenv(const char *name, const char *value, int overwrite);

#define getpid _getpid
#define popen _popen
#define pclose _pclose


#endif

#include "ssize_t.h"
#endif
