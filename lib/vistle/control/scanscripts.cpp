#include "scanscripts.h"
#include <iostream>
#include <filesystem>
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
#ifdef _WIN32
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
    namespace fs = std::filesystem;
    for (const auto &path: scriptsAndDirs) {
        fs::path p(path);
        if (fs::is_directory(p)) {
            std::vector<std::string> dirscripts;
            for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it) {
                fs::path ent(*it);
                if (ent.extension() == vistleScriptExt) {
                    dirscripts.push_back(ent.string());
                }
            }
            std::sort(dirscripts.begin(), dirscripts.end());
            std::copy(dirscripts.begin(), dirscripts.end(), std::back_inserter(scripts));
        } else if (fs::is_regular_file(p)) {
            scripts.push_back(p.string());
        }
    }
    return scripts;
}

} // namespace vistle
