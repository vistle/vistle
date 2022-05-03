#ifndef VISTLE_INSITU_SLOW_MPI_H
#define VISTLE_INSITU_SLOW_MPI_H

#include "export.h"
#include <atomic>

namespace boost {
namespace mpi {
class communicator;
}
} // namespace boost

namespace vistle {
namespace insitu {
//these functions trade latency for less cpu usage
void V_INSITUCOREEXPORT waitForRank(const boost::mpi::communicator &c, int rank);
void V_INSITUCOREEXPORT broadcast(const boost::mpi::communicator &c, int &val, int root);
void V_INSITUCOREEXPORT broadcast(const boost::mpi::communicator &c, bool &val, int root, bool defaultValue = false,
                                  const std::atomic_bool &abort = false);


} // namespace insitu
} // namespace vistle

#endif //VISTLE_INSITU_SLOW_MPI_H
