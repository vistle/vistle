#include <sstream>
#include <iostream>
#include <cstdlib>

#include <boost/lexical_cast.hpp>

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

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "failed to fork for attaching debugger" << std::endl;
        return false;
    } else if (pid == 0) {
        execlp("attach_debugger_vistle.sh", "attach_debugger_vistle.sh",
               boost::lexical_cast<std::string>(getppid()).c_str(), nullptr);
#if 0
      std::stringstream cmd;
      cmd << "attach_debugger_vistle.sh";
      cmd << " ";
      cmd << getppid();
      const int ret = system(cmd.str().c_str());
      if (ret < 0) {
         std::cerr << "failed to execute debugger" << std::endl;
         exit(1);
      } else {
         exit(ret);
      }
      return false;
#else
        std::cerr << "failed to execute debugger: " << strerror(errno) << std::endl;
        exit(1);
#endif
    } else {
#ifndef __EMSCRIPTEN__
        kill(getpid(), SIGSTOP);
        sleep(1);
#endif
#if 0
      const int wait = 30;
      unsigned int remain = sleep(wait);
      if (remain == 0) {
         std::cerr << "debugger failed to attach during " << wait << " seconds" << std::endl;
         return false;
      }
      sleep(3);
#endif
        return true;
    }
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
