#include "spawnprocess.h"
#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

namespace vistle {

process_handle spawn_process(const std::string &executable, const std::vector<std::string> args) {

#ifdef _WIN32
#error("not implemented")
#else
   const pid_t pid = fork();
   if (pid < 0) {
      std::cerr << "Error when forking for executing " << executable << ": " << strerror(errno) << std::endl;
      return false;
   } else if (pid == 0) {
      std::vector<const char *> a;
      for (const auto &s: args) {
         a.push_back(s.c_str());
      }
      a.push_back(nullptr);
      execvp(executable.c_str(), (char **)a.data());
      std::cout << "Error when executing " << executable << ": " << strerror(errno) << std::endl;
      std::cerr << "Error when executing " << executable << ": " << strerror(errno) << std::endl;
      exit(1);
   }
#endif
   return pid;
}

bool kill_process(process_handle pid) {

#ifdef _WIN32
#error("not implemented")
#else
   return kill(pid, SIGTERM) == 0;
#endif
}

process_handle try_wait() {

#ifdef _WIN32
#else
   int stat = 0;
   pid_t pid = wait3(&stat, WNOHANG, nullptr);
   if (pid > 0)
      return pid;
   else
      return 0;
#endif
}

}


