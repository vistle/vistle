#ifndef SYSDEP_H
#define SYSDEP_H

#ifdef WIN32
#include <stdio.h>
#include <process.h>
#include <windows.h>


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

#define getpid _getpid

typedef SSIZE_T ssize_t;

#define close closesocket

typedef int socklen_t;

#endif

#endif
