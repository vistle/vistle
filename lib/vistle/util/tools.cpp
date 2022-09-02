#include <sstream>
#include <iostream>
#include <cstdlib>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define HAVE_EXECINFO
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#else
#include <vistle/util/sysdep.h>
#endif

#include "tools.h"

namespace vistle {

std::string backtrace()
{
    std::stringstream str;

#ifdef HAVE_EXECINFO
    const int MaxFrames = 32;

    void *buffer[MaxFrames] = {0};
    const int count = ::backtrace(buffer, MaxFrames);

    char **symbols = ::backtrace_symbols(buffer, count);

    if (symbols) {
        for (int n = 0; n < count; ++n) {
            str << "   " << symbols[n] << std::endl;
        }

        free(symbols);
    }
#endif

    return str.str();
}

bool attach_debugger()
{
#ifdef _WIN32
    DebugBreak();
    return true;
#else
    //sleep(60);
    abort();
    return true;
#endif
}

bool parentProcessDied()
{
#if defined(_WIN32) || defined(__EMSCRIPTEN__)
    // no implementation
    return false;
#elif defined(__linux__)
    // not necessary as processes die automatically with their parent
    return false;
#else
    return kill(getppid(), 0) == -1;
#endif
}

} // namespace vistle
