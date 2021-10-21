#ifndef VISTLE_AFFINITY_H
#define VISTLE_AFFINITY_H

#include "export.h"

#include <string>

namespace vistle {

V_UTILEXPORT std::string sched_affinity_map();
V_UTILEXPORT std::string thread_affinity_map();

} // namespace vistle
#endif
