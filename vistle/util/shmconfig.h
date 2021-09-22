#ifndef VISTLE_UTIL_SHMCONFIG_H
#define VISTLE_UTIL_SHMCONFIG_H

#include "export.h"

namespace vistle {

bool V_UTILMPIEXPORT
shmPerRank(); // do MPI communication in order to agree whether shmem should be used by all ranks on a node

}
#endif
