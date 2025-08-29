 #pragma once
#include "ThirdPartyHeadersBegin.h"
 #if defined _WIN32
#include <windows.h>
 #else
#include <thread>
 #if defined __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
 #endif
 #endif
#include <utility>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "Mutex_s.h"
 #if defined _WIN32
typedef PTP_WORK thread_t;
 #else
typedef std::thread thread_t;
 #endif
struct ___2120 { ___2662 ___2492; std::vector<thread_t> ___2646; static int ___2825(); ___2120() { ___2492 = new ___2663(); } ~___2120() { delete ___2492; } struct ThreadJobData { ___4158 m_job; ___90 m_jobData; ThreadJobData(___4158 ___2116, ___90 ___2121) : m_job(___2116) , m_jobData(___2121) {} };
 #if defined _WIN32
static VOID CALLBACK ___2120::___4160( PTP_CALLBACK_INSTANCE Instance, PVOID                 Context, PTP_WORK              Work);
 #else
static void *___4160(void* data);
 #endif
void addJob(___4158 ___2116, ___90 ___2121); void wait(); void lock() { ___2492->lock(); } void unlock() { ___2492->unlock(); } }; inline int ___2120::___2825() { int ___2826 = 0;
 #if defined _WIN32
SYSTEM_INFO sysinfo; GetSystemInfo(&sysinfo); ___2826 = static_cast<int>(sysinfo.dwNumberOfProcessors);
 #elif defined __APPLE__
int nm[2]; size_t len = 4; uint32_t count; nm[0] = CTL_HW; nm[1] = HW_AVAILCPU; sysctl(nm, 2, &count, &len, NULL, 0); if(count < 1) { nm[1] = HW_NCPU; sysctl(nm, 2, &count, &len, NULL, 0); if(count < 1) count = 1; } ___2826 = static_cast<int>(count);
 #else
___2826 = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
 #endif
return ___2826; }
 #if defined _WIN32
inline VOID CALLBACK ___2120::___4160( PTP_CALLBACK_INSTANCE, PVOID data, PTP_WORK)
 #else
inline void *___2120::___4160(void* data)
 #endif
{ ThreadJobData* ___2121 = reinterpret_cast<ThreadJobData*>(data); ___2121->m_job(___2121->m_jobData); delete ___2121;
 #if !defined _WIN32
return NULL;
 #endif
} inline void ___2120::addJob(___4158 ___2116, ___90 ___2121) { lock(); ___2120::ThreadJobData* threadJobData = new ThreadJobData(___2116, ___2121);
 #if defined _WIN32
thread_t thr = CreateThreadpoolWork(___4160, threadJobData, NULL); SubmitThreadpoolWork(thr); ___2646.push_back(thr);
 #else
___2646.emplace_back(___4160, (void*)threadJobData);
 #endif
unlock(); } inline void ___2120::wait() { size_t i; for(i = 0; i < ___2646.size(); ++i) { lock(); thread_t thr; std::swap(thr, ___2646[i]); unlock();
 #if defined _WIN32
WaitForThreadpoolWorkCallbacks(thr, ___1303); CloseThreadpoolWork(thr);
 #else
thr.join();
 #endif
} lock(); ___2646.erase(___2646.begin(), ___2646.begin() + i); unlock(); }
