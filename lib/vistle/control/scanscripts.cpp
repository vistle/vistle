#include "scanscripts.h"
#include <iostream>
#include <vistle/util/filesystem.h>
#include <vistle/util/directory.h>
#include <boost/algorithm/string.hpp>

namespace vistle {

namespace {

const char *vistleScriptExt = ".vsl";

std::vector<std::string> scanPreloadScripts()
{
    auto extraDirs = getenv("VISTLE_PRELOAD_SCRIPTS");
    std::vector<std::string> v;
    if (extraDirs) {
#ifdef WIN32
        boost::split(v, extraDirs, boost::is_any_of(";"));
#else
        boost::split(v, extraDirs, boost::is_any_of(":"));
#endif
    }
    return v;
}

} // namespace

std::vector<std::string> scanStartupScripts()
{
    auto scriptsAndDirs = scanPreloadScripts();
    scriptsAndDirs.push_back(directory::configHome());
    std::vector<std::string> scripts;
    namespace bf = vistle::filesystem;
    for (const auto &path: scriptsAndDirs) {
        bf::path p(path);
        if (bf::is_directory(p)) {
            std::vector<std::string> dirscripts;
            for (bf::directory_iterator it(p); it != bf::directory_iterator(); ++it) {
                bf::path ent(*it);
                if (ent.extension() == vistleScriptExt) {
                    dirscripts.push_back(ent.string());
                }
            }
            std::sort(dirscripts.begin(), dirscripts.end());
            std::copy(dirscripts.begin(), dirscripts.end(), std::back_inserter(scripts));
        } else if (bf::is_regular_file(p)) {
            scripts.push_back(p.string());
        }
    }
    return scripts;
}

} // namespace vistle
