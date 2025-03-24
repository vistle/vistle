#ifndef VISTLE_CONTROL_SCANMODULES_H
#define VISTLE_CONTROL_SCANMODULES_H

#include <vistle/core/availablemodule.h>

#include "export.h"
#include <istream>
#include <string>

namespace vistle {

V_HUBEXPORT bool scanModules(const std::string &prefix, const std::string &buildtype, int hub, AvailableMap &available);

} // namespace vistle
#endif
