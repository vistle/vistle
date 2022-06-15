#ifndef VISTLE_AFFINITY_H
#define VISTLE_AFFINITY_H

#include "export.h"

#include <string>

namespace vistle {

V_UTILEXPORT std::string sched_affinity_map();
V_UTILEXPORT std::string thread_affinity_map();
V_UTILEXPORT std::string hwloc_affinity_map(int flags = -1);
V_UTILEXPORT bool apply_affinity_from_environment(int nodeRank, int ranksOnThisNode);

} // namespace vistle
#endif
