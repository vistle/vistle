#ifndef VISTLE_INSITU_SLOW_MPI_H
#define VISTLE_INSITU_SLOW_MPI_H

#include "export.h"
namespace boost {
namespace mpi {
class communicator;
}
} // namespace boost

namespace vistle {
namespace insitu {
//these functions trade latency for less cpu usage
void V_INSITUCOREEXPORT waitForRank(boost::mpi::communicator &c, int rank);
void V_INSITUCOREEXPORT broadcast(boost::mpi::communicator &c, int &val, int root);
void V_INSITUCOREEXPORT broadcast(boost::mpi::communicator &c, bool &val, int root);


} // namespace insitu
} // namespace vistle

#endif //VISTLE_INSITU_SLOW_MPI_H
