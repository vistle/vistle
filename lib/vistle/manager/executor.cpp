/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <vistle/util/sysdep.h>
#include <vistle/util/hostname.h>
#include <vistle/util/shmconfig.h>
#include <vistle/util/affinity.h>

#include <sys/types.h>

#include <cstdlib>
#include <cstdio>

#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/mpi.hpp>
#include <boost/lexical_cast.hpp>

#include <vistle/core/message.h>
#include <vistle/core/messagequeue.h>
#include <vistle/core/object.h>
#include <vistle/core/shm.h>

#include "communicator.h"
#include "executor.h"

namespace interprocess = boost::interprocess;
namespace mpi = boost::mpi;

namespace vistle {

using namespace message;

Executor::Executor(int argc, char *argv[], boost::mpi::communicator comm)
: m_name("vistle"), m_rank(-1), m_size(-1), m_comm(NULL), m_argc(argc), m_argv(argv)
{
    m_size = comm.size();
    m_rank = comm.rank();

    if (argc < 4) {
        std::cerr << "usage: " << argv[0] << " [hostname] [port] [dataPort]" << std::endl;
        std::cerr << "  hostname and port where Vistle hub can be reached have to be specified" << std::endl;
        exit(1);
    }

    unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
    m_name = Shm::instanceName(argv[1], port);
    unsigned short dataPort = boost::lexical_cast<unsigned short>(argv[3]);

    // broadcast name of vistle session
    mpi::broadcast(comm, m_name, 0);

    bool shmPerRank = vistle::shmPerRank();

    std::string hostname = vistle::hostname();
    std::vector<std::string> hostnames;
    mpi::all_gather(comm, hostname, hostnames);

    // determine first rank on each host
    bool first = true;
    std::map<std::string, int> ranksPerNode;
    std::vector<int> nodeRank(m_size);
    for (int index = 0; index < m_size; index++) {
        const auto &h = hostnames[index];
        nodeRank[index] = ranksPerNode[h];
        ++ranksPerNode[h];
        if (h == hostname) {
            if (index < m_rank) {
                first = false;
            }
        }
    }
    if (ranksPerNode[hostname] == 0)
        ranksPerNode[hostname] = 1;
    if (shmPerRank)
        first = true;

    if (first) {
        vistle::Shm::remove(m_name, 0, m_rank, shmPerRank);
        vistle::Shm::create(m_name, 0, m_rank, shmPerRank);
        vistle::Shm::the().setNumRanksOnThisNode(ranksPerNode[hostname]);
        vistle::Shm::the().setNodeRanks(nodeRank);
    }
    comm.barrier();
    if (!first)
        vistle::Shm::attach(m_name, 0, m_rank, shmPerRank);
    comm.barrier();

    vistle::apply_affinity_from_environment(nodeRank[m_rank], ranksPerNode[hostname]);

    m_comm = new vistle::Communicator(m_rank, hostnames, comm);
    if (!m_comm->connectHub(argv[1], port, dataPort)) {
        std::stringstream err;
        err << "failed to connect to Vistle hub on " << argv[1] << ":" << port;
        throw vistle::exception(err.str());
    }
}

Executor::~Executor()
{
    delete m_comm;

    Shm::the().detach();
}

void Executor::setVistleRoot(const std::string &dir)
{
    return m_comm->setVistleRoot(dir);
}


bool Executor::config(int argc, char *argv[])
{
    return true;
}

void Executor::run()
{
    if (!config(m_argc, m_argv))
        return;

    m_comm->run();
}

} // namespace vistle
