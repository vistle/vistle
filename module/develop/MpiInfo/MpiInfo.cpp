#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <vistle/module/module.h>
#include <vistle/core/vec.h>
#include <vistle/core/scalars.h>
#include <vistle/core/message.h>
#include <vistle/core/coords.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/indexed.h>
#include <vistle/util/hostname.h>
#include <vistle/util/affinity.h>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace vistle;

class MpiInfo: public vistle::Module {
public:
    MpiInfo(const std::string &name, int moduleID, mpi::communicator comm);
    ~MpiInfo() override;

private:
    bool prepare() override;
};

using namespace vistle;

MpiInfo::MpiInfo(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    std::stringstream str;
    str << "ctor: rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
    sendInfo(str.str());
}

MpiInfo::~MpiInfo() = default;

bool MpiInfo::prepare()
{
    std::stringstream str;
    str << "prepare(): rank " << rank() << "/" << size() << " on host " << hostname() << std::endl;
#ifdef __linux__
    str << "Process affinity: " << sched_affinity_map() << std::endl;
    str << "Thread affinity:  " << thread_affinity_map() << std::endl;
#endif
    str << "hwloc affinity:   " << hwloc_affinity_map() << std::endl;
#ifdef NO_SHMEM
    str << "Shared memory:    message queues only" << std::endl;
#else
    if (Shm::perRank())
        str << "Shared memory:    no sharing between ranks on same node" << std::endl;
    else
        str << "Shared memory:    common for " << commShmGroup().size() << " ranks on same node, lead by "
            << shmLeader() << std::endl;
#endif

    if (rank() == 0) {
        std::stringstream str;
#ifdef _OPENMP
        std::unordered_map<unsigned, std::string> map{
            {199810, "1.0"}, {200203, "2.0"}, {200505, "2.5"}, {200805, "3.0"}, {201107, "3.1"}, {201307, "4.0"},
            {201511, "4.5"}, {201811, "5.0"}, {202011, "5.1"}, {202111, "5.2"}, {202411, "6.0"}};
        str << "OpenMP: version " << map.at(_OPENMP) << " enabled with up to " << omp_get_max_threads() << " threads"
            << std::endl;
#else
        str << "OpenMP: not enabled" << std::endl;
#endif
        sendInfo(str.str());
    }

    if (rank() == 0) {
        int len = 0;
        char version[MPI_MAX_LIBRARY_VERSION_STRING];
        MPI_Get_library_version(version, &len);
        std::stringstream str;
        str << "MPI version: " << std::string(version, len) << std::endl;
        sendInfo(str.str());
    }

    return true;
}

MODULE_MAIN(MpiInfo)
