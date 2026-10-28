#include "scanmodules.h"
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <filesystem>
#include <vistle/util/directory.h>
#include <vistle/module_descriptions/descriptions.h>

namespace vistle {

bool scanModules(const std::string &prefix, const std::string &buildtype, int hub, AvailableMap &available)
{
    namespace fs = std::filesystem;
    auto dir = Directory(prefix, buildtype);
    auto share = dir.share();

    std::map<std::string, ModuleDescription> moduleDescriptions = getModuleDescriptions(share);

    auto moddir = dir.module();
    fs::path p(moddir);
    try {
        if (!fs::is_directory(p)) {
            std::cerr << "scanModules: " << moddir << " is not a directory" << std::endl;
            return false;
        }
    } catch (const fs::filesystem_error &e) {
        std::cerr << "scanModules: error in" << moddir << ": " << e.what() << std::endl;
        return false;
    }

    p = fs::canonical(p);

    //std::cerr << "scanModules: looking for modules in " << p << std::endl;

    for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it) {
        fs::path ent(*it);
        std::string stem = ent.stem().string();
        if (stem.size() > ModuleNameLength) {
            std::cerr << "scanModules: skipping " << stem << " - name too long" << std::endl;
            continue;
        }
        if (stem.empty()) {
            continue;
        }
        boost::system::error_code ec;
        if (fs::is_directory(ent, ec)) {
            continue;
        }
        if (ec) {
            std::cerr << "scanModules: skipping " << stem << " - error: " << ec.message() << std::endl;
            continue;
        }

#ifdef MODULE_THREAD
        std::string ext = ent.extension().string();
#ifdef _WIN32
        if (ext != ".dll") {
            //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
            continue;
        }
#else
        if (ext != ".so") {
            //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
            continue;
        }
#endif
#else
#ifdef _WIN32
        std::string ext = ent.extension().string();
        if (ext != ".exe") {
            //std::cerr << "scanModules: skipping " << stem << ": ext=" << ext << std::endl;
            continue;
        }
#endif
#endif

        std::string name;
#ifdef MODULE_THREAD
        name = stem;
        if (name.find("libmod_") == 0)
            name = name.substr(7);
#else
        name = stem;
#endif
        std::string cat = "Unspecified";
        std::string desc = "";
        auto descit = moduleDescriptions.find(name);
        if (descit != moduleDescriptions.end()) {
            cat = descit->second.category;
            desc = descit->second.description;
        }
        AvailableModule mod{hub, name, fs::path(*it).string(), cat, desc};

        AvailableModule::Key key(hub, name);
        auto prev = available.find(key);
        if (prev == available.end()) {
            available[key] = std::move(mod);
        } else {
            std::cerr << "scanModules: overriding " << name << ", " << prev->second.path() << " -> " << mod.path()
                      << std::endl;
            prev->second = std::move(mod);
        }
    }

    return true;
}

} // namespace vistle
