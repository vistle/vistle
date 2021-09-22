#include "affinity.h"
#include <sstream>
#include <cstring>
#include <cerrno>
#include <iostream>

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <sys/sysinfo.h>
#define SHOW_AFFINITY
#endif

namespace vistle {

#ifdef SHOW_AFFINITY
namespace {

std::string cpuset_to_map(const cpu_set_t &set, int nprocs = -1)
{
    std::stringstream str;
    for (int i = 0; i < CPU_SETSIZE && (nprocs < 0 || i < nprocs); ++i) {
        if (CPU_ISSET(i, &set))
            str << i % 10;
        else
            str << ".";
    }
    return str.str();
}

int get_num_cpus()
{
    return get_nprocs_conf();
}

} // namespace
#endif

std::string sched_affinity_map()
{
#ifdef SHOW_AFFINITY
    cpu_set_t set;
    CPU_ZERO(&set);
    int err = sched_getaffinity(0 /* this process */, sizeof(set), &set);
    if (err == -1) {
        std::cerr << "failed to query CPU affinity via sched_getaffinity: " << strerror(errno) << std::endl;
        return std::string("ERROR: ") + strerror(errno);
    }

    int nprocs = get_num_cpus();
    return cpuset_to_map(set, nprocs);
#else
    return std::string("n/a");
#endif
}

std::string thread_affinity_map()
{
#ifdef SHOW_AFFINITY
    cpu_set_t set;
    CPU_ZERO(&set);
    int err = pthread_getaffinity_np(pthread_self(), sizeof(set), &set);
    if (err == -1) {
        std::cerr << "failed to query CPU affinity via pthread_getaffinity_np: " << strerror(errno) << std::endl;
        return std::string("ERROR: ") + strerror(errno);
    }

    int nprocs = get_num_cpus();
    return cpuset_to_map(set, nprocs);
#else
    return std::string("n/a");
#endif
}

} // namespace vistle
