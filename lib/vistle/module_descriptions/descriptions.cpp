#include "descriptions.h"
#include <vistle/util/filesystem.h>
#include <iostream>

#ifdef MODULE_STATIC
#include <cmrc/cmrc.hpp>
#include <sstream>
#else
#include <fstream>
#endif

namespace vistle {

#ifdef MODULE_STATIC
CMRC_DECLARE(moduledescriptions);
#endif

std::map<std::string, ModuleDescription> getModuleDescriptions(const std::string &share_prefix)
{
    std::map<std::string, ModuleDescription> moduleDescriptions;
#ifdef MODULE_STATIC
    try {
        auto fs = cmrc::moduledescriptions::get_filesystem();
        auto data = fs.open("moduledescriptions.txt");
        std::string desc(data.begin(), data.end());
        std::stringstream str(desc);
        moduleDescriptions = readModuleDescriptions(str);
    } catch (std::exception &ex) {
        std::cerr << "getModuleDescriptions: exception: " << ex.what() << std::endl;
    }
#else
    filesystem::path pshare(share_prefix);
    try {
        if (!filesystem::is_directory(pshare)) {
            std::cerr << "scanModules: " << share_prefix << " is not a directory" << std::endl;
        } else {
        }
    } catch (const filesystem::filesystem_error &e) {
        std::cerr << "scanModules: error in" << share_prefix << ": " << e.what() << std::endl;
    }
    std::string file = share_prefix + "moduledescriptions.txt";
    std::fstream str(file, std::ios_base::in);
    if (str.is_open()) {
        moduleDescriptions = readModuleDescriptions(str);
    } else {
        std::cerr << "getModuleDescriptions: failed to open module description file " << file << std::endl;
    }
#endif
    return moduleDescriptions;
}


std::map<std::string, ModuleDescription> readModuleDescriptions(std::istream &str)
{
    std::map<std::string, ModuleDescription> moduleDescriptions;

    std::string line;
    while (std::getline(str, line)) {
        auto sep = line.find(' ');
        auto mod = line.substr(0, sep);
        line = line.substr(sep + 1);
        sep = line.find(' ');
        auto cat = line.substr(0, sep);
        auto desc = line.substr(sep + 1);
        moduleDescriptions[mod].category = cat;
        moduleDescriptions[mod].description = desc;
        //std::cerr << "module: " << mod << " -> " << cat << ", description: " << cat << std::endl;
    }
    return moduleDescriptions;
}

} // namespace vistle
