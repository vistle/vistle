#ifndef VISTLE_SCANMODULES_H
#define VISTLE_SCANMODULES_H

#include <vistle/core/availablemodule.h>

#include "export.h"
#include <istream>
#include <string>

namespace vistle {

struct ModuleDescription {
    std::string category;
    std::string description;
};

V_HUBEXPORT std::map<std::string, ModuleDescription> readModuleDescriptions(std::istream &str);

V_HUBEXPORT bool scanModules(const std::string &prefix, const std::string &buildtype, int hub, AvailableMap &available);

} // namespace vistle
#endif
