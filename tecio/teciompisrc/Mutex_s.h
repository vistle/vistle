 #pragma once
#include "ThirdPartyHeadersBegin.h"
 #if defined _WIN32
#include <windows.h>
 #else
#include <thread>
#include <mutex>
 #if defined __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
 #endif
 #endif
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
struct ___2663 {
 #if defined _WIN32
HANDLE ___2492;
 #else
std::mutex ___2492;
 #endif
___2663() {
 #if defined _WIN32
___2492 = CreateMutex(NULL, ___1303, NULL);
 #endif
} ~___2663() {
 #if defined _WIN32
CloseHandle(___2492);
 #endif
} void lock() {
 #if defined _WIN32
WaitForSingleObject(___2492, INFINITE);
 #else
___2492.lock();
 #endif
} void unlock() {
 #if defined _WIN32
ReleaseMutex(___2492);
 #else
___2492.unlock();
 #endif
} };
