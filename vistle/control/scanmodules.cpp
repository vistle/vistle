#include <iostream>
#include <vistle/util/filesystem.h>
#include <vistle/util/directory.h>
#include "scanmodules.h"

namespace vistle {

bool scanModules(const std::string &dir, int hub, AvailableMap &available)
{
    namespace bf = vistle::filesystem;
    using vistle::directory::build_type;
    bf::path p(dir);
    if (!build_type().empty()) {
#ifdef MODULE_THREAD
        std::cerr << dir + "/../../../lib/module/" + build_type() << std::endl;
        p = dir + "/../../../lib/module/" + build_type();
#else
        std::cerr << dir + "/../../../libexec/module/" + build_type() << std::endl;
        p = dir + "/../../../libexec/module/" + build_type();
#endif
    }
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

        AvailableModule mod;
        mod.hub = hub;
#ifdef MODULE_THREAD
        if (stem.find("lib") == 0)
            mod.name = stem.substr(3);
        else
            mod.name = stem;
#else
        mod.name = stem;
#endif
        mod.path = bf::path(*it).string();

        AvailableModule::Key key(hub, mod.name);
        //std::cerr << "scanModules: new " << mod.name << " on hub " << hub << std::endl;
        auto prev = available.find(key);
        if (prev != available.end()) {
            std::cerr << "scanModules: overriding " << mod.name << ", " << prev->second.path << " -> " << mod.path
                      << std::endl;
        }
        available[key] = mod;
    }

    return true;
}

}
