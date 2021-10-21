#include "shmconfig.h"
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#include <stdlib.h>

namespace vistle {

bool shmPerRank()
{
#ifndef NO_SHMEM
    const char *env = getenv("VISTLE_SHM_PER_RANK");
    bool perRank = env ? true : false;
    boost::mpi::broadcast(boost::mpi::communicator(), perRank, 0);
    return perRank;
#else
    return false;
#endif
}

} // namespace vistle
