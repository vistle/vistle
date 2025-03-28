#ifndef VISTLE_MODULE_DESCRIPTIONS_DESCRIPTIONS_H
#define VISTLE_MODULE_DESCRIPTIONS_DESCRIPTIONS_H

#include "export.h"

#include <map>
#include <string>
#include <vector>

namespace vistle {

struct ModuleDescription {
    std::string category;
    std::string description;
};

V_MODULEDESCRIPTIONSEXPORT std::map<std::string, ModuleDescription>
getModuleDescriptions(const std::string &share_prefix);

V_MODULEDESCRIPTIONSEXPORT std::vector<std::string> getModuleCategories(const std::string &share_prefix);
V_MODULEDESCRIPTIONSEXPORT
std::map<std::string, std::string> getCategoryDescriptions(const std::string &share_prefix);

} // namespace vistle
#endif
