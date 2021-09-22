#include "sysdep.h"
#ifdef WIN32
#include <stdlib.h>
#include <Windows.h>
#include <sddl.h>

#include <iostream>
#include <iomanip>
#include <memory>

void sleep(int x)
{
    Sleep(x * 1000);
}

void usleep(__int64 usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}

int setenv(const char *name, const char *value, int overwrite)
{
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize)
            return errcode;
    }
    return _putenv_s(name, value);
}

struct heap_delete {
    typedef LPVOID pointer;
    void operator()(LPVOID p) { ::HeapFree(::GetProcessHeap(), 0, p); }
};
typedef std::unique_ptr<LPVOID, heap_delete> heap_unique_ptr;

struct handle_delete {
    typedef HANDLE pointer;
    void operator()(HANDLE p) { ::CloseHandle(p); }
};
typedef std::unique_ptr<HANDLE, handle_delete> handle_unique_ptr;


BOOL GetUserSID(HANDLE token, PSID *sid)
{
    if (token == nullptr || token == INVALID_HANDLE_VALUE || sid == nullptr) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    DWORD tokenInformationLength = 0;
    ::GetTokenInformation(token, TokenUser, nullptr, 0, &tokenInformationLength);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
    }
    heap_unique_ptr data(::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tokenInformationLength));
    if (data.get() == nullptr) {
        return FALSE;
    }
    BOOL getTokenInfo =
        ::GetTokenInformation(token, TokenUser, data.get(), tokenInformationLength, &tokenInformationLength);
    if (!getTokenInfo) {
        return FALSE;
    }
    PTOKEN_USER pTokenUser = (PTOKEN_USER)(data.get());
    DWORD sidLength = ::GetLengthSid(pTokenUser->User.Sid);
    heap_unique_ptr sidPtr(::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sidLength));
    PSID sidL = (PSID)(sidPtr.get());
    if (sidL == nullptr) {
        return FALSE;
    }
    BOOL copySid = ::CopySid(sidLength, sidL, pTokenUser->User.Sid);
    if (!copySid) {
        return FALSE;
    }
    if (!IsValidSid(sidL)) {
        return FALSE;
    }
    *sid = sidL;
    sidPtr.release();
    return TRUE;
}

uid_t GetUID(HANDLE token)
{
    PSID sid = nullptr;
    BOOL getSID = GetUserSID(token, &sid);
    if (!getSID || !sid) {
        return -1;
    }
    heap_unique_ptr sidPtr((LPVOID)(sid));
    LPWSTR stringSid = nullptr;
    BOOL convertSid = ::ConvertSidToStringSidW(sid, &stringSid);
    if (!convertSid) {
        return -1;
    }
    uid_t ret = -1;
    LPCWSTR p = ::wcsrchr(stringSid, L'-');
    if (p && ::iswdigit(p[1])) {
        ++p;
        ret = ::_wtoi(p);
    }
    ::LocalFree(stringSid);
    return ret;
}

uid_t getuid()
{
    HANDLE process = ::GetCurrentProcess();
    handle_unique_ptr processPtr(process);
    HANDLE token = nullptr;
    BOOL openToken = ::OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token);
    if (!openToken) {
        return -1;
    }
    handle_unique_ptr tokenPtr(token);
    uid_t ret = GetUID(token);
    return ret;
}

uid_t geteuid()
{
    HANDLE process = ::GetCurrentProcess();
    HANDLE thread = ::GetCurrentThread();
    HANDLE token = nullptr;
    BOOL openToken = ::OpenThreadToken(thread, TOKEN_READ | TOKEN_QUERY_SOURCE, FALSE, &token);
    if (!openToken && ::GetLastError() == ERROR_NO_TOKEN) {
        openToken = ::OpenThreadToken(thread, TOKEN_READ | TOKEN_QUERY_SOURCE, TRUE, &token);
        if (!openToken && ::GetLastError() == ERROR_NO_TOKEN) {
            openToken = ::OpenProcessToken(process, TOKEN_READ | TOKEN_QUERY_SOURCE, &token);
        }
    }
    if (!openToken) {
        return -1;
    }
    handle_unique_ptr tokenPtr(token);
    uid_t ret = GetUID(token);
    return ret;
}
#endif
