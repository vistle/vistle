#include "spawnprocess.h"
#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <list>
#include <process.h>
#include <Windows.h>
#include <WinBase.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define USE_POSIX_SPAWN
#ifdef USE_POSIX_SPAWN
#include <spawn.h>
#endif
#endif

#ifdef __linux__
#include <sys/prctl.h>
#endif

#ifdef __APPLE__
#include <crt_externs.h>
#endif

namespace vistle {

#ifdef _WIN32
static	std::list<process_handle> childProcesses;
#endif

process_handle spawn_process(const std::string &executable, const std::vector<std::string> args) {
    std::vector<const char *> a;
    for (const auto &s : args) {
        a.push_back(s.c_str());
    }
    a.push_back(nullptr);
#ifdef _WIN32
   process_handle pid = _spawnvp(P_NOWAIT, executable.c_str(), (char *const *)&a[0]);
   if (pid == -1) {
       std::cerr << "Error when spawning " << executable << ": " << strerror(errno) << std::endl;
       std::cerr << "PATH is " << getenv("PATH") << std::endl;
   }
   childProcesses.push_back(pid);
#else
#ifdef USE_POSIX_SPAWN
   pid_t pid = 0;
   posix_spawnattr_t attr;
   posix_spawnattr_init(&attr);
   posix_spawn_file_actions_t file_actions;
   posix_spawn_file_actions_init(&file_actions);
#ifdef __APPLE__
   char ***environp = _NSGetEnviron();
   if (!environp) {
       std::cerr << "Error when spawning " << executable << ": failed to get environment with _NSGetEnviron()" << std::endl;
       return 0;
   }
   char **environ = *environp;
#endif
   int err = posix_spawnp(&pid, executable.c_str(), &file_actions, &attr, (char **)a.data(), environ);
   posix_spawnattr_destroy(&attr);
   posix_spawn_file_actions_destroy(&file_actions);
   if (err != 0) {
      std::cerr << "Error in posix_spawn for " << executable << ": " << strerror(errno) << std::endl;
      return -1;
   }
   return pid;
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
#endif
   return pid;
}

void replace_process(const std::string &executable, const std::vector<std::string> args) {
    std::vector<const char *> a;
    for (const auto &s : args) {
        a.push_back(s.c_str());
    }
    a.push_back(nullptr);
#ifdef _WIN32
    std::cerr << "Error when executing " << executable << ": not implemented" << std::endl;
#else
    execvp(executable.c_str(), (char **)a.data());
    std::cout << "Error when executing " << executable << ": " << strerror(errno) << std::endl;
    std::cerr << "Error when executing " << executable << ": " << strerror(errno) << std::endl;
#endif
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
	for (auto it = childProcesses.begin();it != childProcesses.end();it++)
	{
		DWORD exitCode;
		process_handle pid = *it;
		if (GetExitCodeProcess((HANDLE)pid, &exitCode))
		{
			if (exitCode != STILL_ACTIVE)
			{
				childProcesses.erase(it);
				return(pid);
			}
		}
	}

	
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
	DWORD exitCode;
	if (GetExitCodeProcess((HANDLE)pid0, &exitCode))
	{
		if (exitCode != STILL_ACTIVE)
		{
			childProcesses.remove(pid0);
			return(pid0);
		}
	}
	return 0;
#else
   int stat = 0;
   pid_t pid = wait4(pid0, &stat, WNOHANG, nullptr);
   if (WIFEXITED(stat) || WIFSIGNALED(stat)) {
       if (status)
           *status = stat;
       if (pid > 0)
           return pid;
   }

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


