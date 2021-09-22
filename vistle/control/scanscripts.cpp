#include "scanscripts.h"
#include <iostream>
#include <vistle/util/filesystem.h>
#include <vistle/util/directory.h>
#include <boost/algorithm/string.hpp>

using namespace vistle;

const char *vistleScriptExt = ".vsl";

std::vector<std::string> scanPreloadScripts()
{
    auto extraDirs = getenv("VISTLE_PRELOAD_SCRIPTS");
    std::vector<std::string> v;
    if (extraDirs) {
        boost::split(v, extraDirs, boost::is_any_of(",:"));
    }
    return v;
}

std::vector<std::string> vistle::scanStartupScripts()
{
    auto scriptsAndDirs = scanPreloadScripts();
    scriptsAndDirs.push_back(directory::configHome());
    std::vector<std::string> scripts;
    namespace bf = vistle::filesystem;
    for (const auto &path: scriptsAndDirs) {
        bf::path p(path);
        if (bf::is_directory(p)) {
            for (bf::directory_iterator it(p); it != bf::directory_iterator(); ++it) {
                bf::path ent(*it);
                if (ent.extension() == vistleScriptExt) {
                    scripts.push_back(ent.string());
                }
            }
        } else if (bf::is_regular_file(p)) {
            scripts.push_back(p.string());
        }
    }
    return scripts;
}
