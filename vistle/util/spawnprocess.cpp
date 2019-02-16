#include "spawnprocess.h"
#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <process.h>
#include <Windows.h>
#include <WinBase.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace vistle {

process_handle spawn_process(const std::string &executable, const std::vector<std::string> args) {
	std::vector<const char *> a;
	for (const auto &s : args) {
		a.push_back(s.c_str());
	}
	a.push_back(nullptr);
#ifdef _WIN32
   process_handle pid = _spawnvp(P_NOWAIT, a[0], (char *const *)&a[1]);
   if (pid == -1) {
      std::cerr << "Error when spawning " << executable << ": " << strerror(errno) << std::endl;
	  std::cerr << "PATH is " << getenv("PATH") << std::endl;
   }
#else
   const pid_t ppid = getpid();
   const pid_t pid = fork();
   if (pid < 0) {
      std::cerr << "Error when forking for executing " << executable << ": " << strerror(errno) << std::endl;
      return -1;
   } else if (pid == 0) {  
#ifdef __linux__
      if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
          std::cerr << "Error when setting parent-death signal: " << strerror(errno) << std::endl;
      }
#endif
      if (getppid() != ppid) {
          std::cout << "Parent died before executing " << executable << ": " << strerror(errno) << std::endl;
          exit(1);
      }
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
   return TerminateProcess((HANDLE)pid, -9) != 0;
#else
   return kill(pid, SIGTERM) == 0;
#endif
}

process_handle try_wait(int *status) {

#ifdef _WIN32
	return 0;
#else
   int stat = 0;
   pid_t pid = wait3(&stat, WNOHANG, nullptr);
   if (status)
       *status = stat;
   if (pid > 0)
      return pid;

   return 0;
#endif
}

process_handle try_wait(process_handle pid0, int *status) {

#ifdef _WIN32
    return 0;
#else
   int stat = 0;
   pid_t pid = wait4(pid0, &stat, WNOHANG, nullptr);
   if (status)
       *status = stat;
   if (pid > 0)
      return pid;

   return 0;
#endif
}

process_handle get_process_handle() {
#ifdef _WIN32
    return _getpid();
#else
    return getpid();
#endif
}

}


