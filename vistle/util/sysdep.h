#ifndef SYSDEP_H
#define SYSDEP_H

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

inline void sleep(int x) { Sleep(x*1000); }; 
inline void usleep(__int64 usec) 
{ 
    HANDLE timer; 
    LARGE_INTEGER ft; 

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
};
inline int setenv(const char *name, const char *value, int overwrite)
{
	int errcode = 0;
	if (!overwrite) {
		size_t envsize = 0;
		errcode = getenv_s(&envsize, NULL, 0, name);
		if (errcode || envsize) return errcode;
	}
	return _putenv_s(name, value);
}

#define getpid _getpid
#define popen _popen
#define pclose _pclose

typedef SSIZE_T ssize_t;

#endif

#endif
