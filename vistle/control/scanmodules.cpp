#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <vistle/util/filesystem.h>
#include <vistle/util/directory.h>
#include "scanmodules.h"

namespace vistle {

std::map<std::string, std::string> readModuleDescriptions(std::istream &str)
{
    std::map<std::string, std::string> moduleDescriptions;

    std::string line;
    while (std::getline(str, line)) {
        auto del = line.find_first_of(" ");
        moduleDescriptions[line.substr(0, del)] = line.substr(del + 1, line.size());
        //std::cerr << "module: " << line.substr(0, del) << " -> description: " << line.substr(del+1, line.size()) << std::endl;
    }
    return moduleDescriptions;
}


bool scanModules(const std::string &prefix, int hub, AvailableMap &available)
{
    namespace bf = vistle::filesystem;
    using vistle::directory::build_type;

    std::map<std::string, std::string> moduleDescriptions;

    auto share = directory::share(prefix);
    bf::path pshare(share);
    try {
        if (!bf::is_directory(pshare)) {
            std::cerr << "scanModules: " << share << " is not a directory" << std::endl;
        } else {
            std::string file = share + "/moduledescriptions.txt";
            std::fstream f(file, std::ios_base::in);

            if (f.is_open()) {
                moduleDescriptions = readModuleDescriptions(f);
            } else {
                std::cerr << "failed to open module description file " << file << std::endl;
            }
        }
    } catch (const bf::filesystem_error &e) {
        std::cerr << "scanModules: error in" << share << ": " << e.what() << std::endl;
    }

    auto dir = directory::module(prefix);
    bf::path p(dir);
    try {
        if (!bf::is_directory(p)) {
            std::cerr << "scanModules: " << dir << " is not a directory" << std::endl;
            return false;
        }
    } catch (const bf::filesystem_error &e) {
        std::cerr << "scanModules: error in" << dir << ": " << e.what() << std::endl;
        return false;
    }

    p = bf::canonical(p);

    //std::cerr << "scanModules: looking for modules in " << p << std::endl;

    for (bf::directory_iterator it(p); it != bf::directory_iterator(); ++it) {
        bf::path ent(*it);
        std::string stem = ent.stem().string();
        if (stem.size() > ModuleNameLength) {
            std::cerr << "scanModules: skipping " << stem << " - name too long" << std::endl;
            continue;
        }
        if (stem.empty()) {
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
        if (stem.find("lib") == 0)
            name = stem.substr(3);
        else
            name = stem;
#else
        name = stem;
#endif
        auto descit = moduleDescriptions.find(name);
        AvailableModule mod{hub, name, bf::path(*it).string(),
                            descit != moduleDescriptions.end() ? descit->second : ""};

        AvailableModule::Key key(hub, name);
        auto prev = available.find(key);
        if (prev != available.end()) {
            std::cerr << "scanModules: overriding " << name << ", " << prev->second.path() << " -> " << mod.path()
                      << std::endl;
        }
        available[key] = std::move(mod);
    }

    return true;
}

} // namespace vistle
