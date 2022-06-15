#include "affinity.h"
#include "coRestraint.h"
#include <sstream>
#include <cstring>
#include <cerrno>
#include <iostream>

#if defined(HAVE_HWLOC)
#include <hwloc.h>
#endif

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#include <sys/sysinfo.h>
#define SHOW_AFFINITY
#endif

namespace vistle {
namespace {

template<class Bitset>
bool is_set(const Bitset &set, int i);

template<class Bitset>
std::string bitset_to_string(const Bitset &set, int maxproc)
{
    std::stringstream bits, runs;
    int run = -2;
    for (int i = 0; i < maxproc; ++i) {
        if (is_set(set, i)) {
            if (run < 0) {
                if (run == -1)
                    runs << ",";
                runs << i;
                run = i;
            }
            bits << i % 10;
        } else {
            if (run >= 0) {
                if (run < i - 1)
                    runs << "-" << i - 1;
                run = -1;
            }
            bits << ".";
        }
    }
    if (run >= 0) {
        if (run < maxproc - 1)
            runs << "-" << maxproc - 1;
    }
    return runs.str();
}

#ifdef HAVE_HWLOC
template<>
bool is_set<hwloc_cpuset_t>(const hwloc_cpuset_t &set, int i)
{
    return hwloc_bitmap_isset(set, i);
}

std::string cpuset_to_string(const hwloc_cpuset_t &set, int nprocs = -1)
{
    std::stringstream bits, runs;
    int maxproc = nprocs < 0 ? hwloc_bitmap_last(set) + 1 : nprocs;
    return bitset_to_string(set, maxproc);
}
#endif

#ifdef SHOW_AFFINITY
template<>
bool is_set<cpu_set_t>(const cpu_set_t &set, int i)
{
    return CPU_ISSET(i, &set);
}

std::string cpuset_to_string(const cpu_set_t &set, int nprocs = -1)
{
    std::stringstream bits, runs;
    int maxproc = nprocs < 0 ? CPU_SETSIZE : nprocs;
    return bitset_to_string(set, maxproc);
}

int get_num_cpus()
{
    return get_nprocs_conf();
}
#endif
} // namespace

std::string hwloc_affinity_map(int flags)
{
    if (flags == -1)
        flags = 0;
#if defined(HAVE_HWLOC)
    flags = HWLOC_CPUBIND_PROCESS;
    hwloc_topology_t topology;
    if (hwloc_topology_init(&topology) == -1) {
        return std::string("hwloc error: topology init");
    }
    hwloc_topology_load(topology);
    hwloc_cpuset_t set = hwloc_bitmap_alloc();
    if (set == nullptr) {
        hwloc_topology_destroy(topology);
        return std::string("hwloc error: bitmap alloc");
    }
    if (hwloc_get_cpubind(topology, set, flags) == -1) {
        hwloc_bitmap_free(set);
        hwloc_topology_destroy(topology);
        return std::string("hwloc error: get cpubind");
    }
    auto str = cpuset_to_string(set);
    hwloc_bitmap_free(set);
    hwloc_topology_destroy(topology);
    return str;
#else
    return std::string("n/a");
#endif
}

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
    return cpuset_to_string(set, nprocs);
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
    return cpuset_to_string(set, nprocs);
#else
    return std::string("n/a");
#endif
}

bool apply_affinity_from_environment(int nodeRank, int ranksOnThisNode)
{
    const char *cenv = getenv("VISTLE_AFFINITY");
    if (!cenv)
        return true;
    if (ranksOnThisNode <= 0)
        return false;
    std::string env(cenv);
#ifdef HAVE_HWLOC
    int depth = -1;
    hwloc_topology_t topology;
    if (hwloc_topology_init(&topology) == -1) {
        return false;
    }
    hwloc_topology_load(topology);
    hwloc_cpuset_t set = hwloc_bitmap_alloc();
    hwloc_cpuset_t excludeset = hwloc_bitmap_alloc();
    hwloc_cpuset_t intersect = hwloc_bitmap_alloc();
    if (set == nullptr || excludeset == nullptr || intersect == nullptr) {
        hwloc_topology_destroy(topology);
        return false;
    }

    std::string affinity;
    auto excludepos = env.find("exclude=");
    if (excludepos != std::string::npos) {
        auto excludeend = env.find(" ", excludepos);
        auto exclude = env.substr(excludepos + 8, excludeend);
        vistle::coRestraint restraint;
        restraint.add(exclude);
        for (int i = 0; i <= restraint.upper(); ++i) {
            if (restraint(i))
                hwloc_bitmap_set(excludeset, i);
        }
        std::cerr << "affinity: excluding CPUs/PUs " << cpuset_to_string(excludeset) << std::endl;
        affinity = "cpu";
    }
    if (env.find("pu") != std::string::npos) {
        affinity = "pu";
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
    }
    if (env.find("cpu") != std::string::npos || env.find("core") != std::string::npos) {
        affinity = "core";
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
    }
    if (env.find("package") != std::string::npos || env.find("chip") != std::string::npos) {
        affinity = "package";
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PACKAGE);
    }
    if (env.find("numa") != std::string::npos) {
        affinity = "numanode";
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE);
    }
    if (env.find("machine") != std::string::npos) {
        affinity = "machine";
        depth = hwloc_get_type_depth(topology, HWLOC_OBJ_MACHINE);
    }
    std::cerr << "affinity: VISTLE_AFFINITY=" << env << ", setting affinity to " << affinity << " (depth " << depth
              << ")" << std::endl;
    if (depth == -1) {
        std::cerr << "affinity: possible values for VISTLE_AFFINITY are "
                  << "pu, core, chip, numa, machinee" << std::endl;
        return false;
    }

    int nunits = hwloc_get_nbobjs_by_depth(topology, depth);
    int nusable = 0;
    bool exclusive = true;
    for (int u = 0; u < nunits; ++u) {
        auto obj = hwloc_get_obj_by_depth(topology, depth, u);
        if (!obj)
            continue;
        auto cpuset = obj->cpuset;
        hwloc_bitmap_and(intersect, cpuset, excludeset);
        if (!hwloc_bitmap_iszero(intersect))
            continue;
        ++nusable;
    }
    if (nusable == 0) {
        exclusive = false;
        for (int u = 0; u < nunits; ++u) {
            auto obj = hwloc_get_obj_by_depth(topology, depth, u);
            if (!obj)
                continue;
            auto cpuset = obj->cpuset;
            hwloc_bitmap_andnot(intersect, cpuset, excludeset);
            if (hwloc_bitmap_iszero(intersect))
                continue;
            ++nusable;
        }
    }
    if (nusable == 0) {
        std::cerr << "affinity: did not find usable " << affinity << " entity" << std::endl;
        return false;
    }

    int unitsPerRank = std::max(1, nusable / ranksOnThisNode);
    int nUnitBlocks = nusable / unitsPerRank;

    std::cerr << "affinity: have " << nunits << " units, " << nusable << " usable for " << ranksOnThisNode
              << " ranks, distributing in blocks of " << unitsPerRank << " units for " << nodeRank
              << "th rank on this node" << std::endl;

    int unitBlockForRank = nodeRank % nUnitBlocks;
    int unitBlock = 0;
    int unitCount = 0;
    hwloc_bitmap_zero(set);
    for (int u = 0; u < nunits; ++u) {
        auto obj = hwloc_get_obj_by_depth(topology, depth, u);
        if (!obj)
            continue;
        auto cpuset = obj->cpuset;
        if (exclusive) {
            hwloc_bitmap_and(intersect, cpuset, excludeset);
            if (!hwloc_bitmap_iszero(intersect))
                continue;
        }
        hwloc_bitmap_andnot(intersect, cpuset, excludeset);
        if (hwloc_bitmap_iszero(intersect))
            continue;

        if (unitBlockForRank == unitBlock) {
            hwloc_bitmap_or(set, set, intersect);
        }

        ++unitCount;
        if (unitCount == unitsPerRank) {
            ++unitBlock;
            unitCount = 0;
        }
    }

    hwloc_set_cpubind(topology, set, 0);
    std::cerr << "affinity: binding to CPUs/PUs " << cpuset_to_string(set) << std::endl;

    hwloc_bitmap_free(set);
    hwloc_bitmap_free(excludeset);
    hwloc_bitmap_free(intersect);
    hwloc_topology_destroy(topology);
    return true;
#endif
    return false;
}

} // namespace vistle
