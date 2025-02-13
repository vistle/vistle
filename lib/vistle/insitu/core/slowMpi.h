#ifndef VISTLE_INSITU_CORE_SLOWMPI_H
#define VISTLE_INSITU_CORE_SLOWMPI_H

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
void V_INSITUCOREEXPORT barrier(const boost::mpi::communicator &c, const std::atomic_bool &abort = false);


} // namespace insitu
} // namespace vistle

#endif
