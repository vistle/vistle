#ifndef VISTLE_SCANMODULES_H
#define VISTLE_SCANMODULES_H

#include <vistle/core/availablemodule.h>

#include "export.h"
#include <istream>

namespace vistle {

V_HUBEXPORT std::map<std::string, std::string> readModuleDescriptions(std::istream &str);

V_HUBEXPORT bool scanModules(const std::string &directory, int hub, AvailableMap &available);

} // namespace vistle
#endif
