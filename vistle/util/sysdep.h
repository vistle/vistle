#ifndef SYSDEP_H
#define SYSDEP_H

#ifdef WIN32
#include <stdio.h>
#include <process.h>
#define NOMINMAX
#include <windows.h>


inline void sleep(int x) { Sleep(x*1000); }; 
inline void usleep(int x) { Sleep(x); }; 

#define getpid _getpid

typedef SSIZE_T ssize_t;

#endif

#endif
