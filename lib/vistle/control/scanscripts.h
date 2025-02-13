#ifndef VISTLE_CONTROL_SCANSCRIPTS_H
#define VISTLE_CONTROL_SCANSCRIPTS_H

#include <vector>
#include <string>
namespace vistle {
//search in user config dir and in dirs/files found in VISTLE_PRELOAD_SCRIPTS
std::vector<std::string> scanStartupScripts();
} // namespace vistle


#endif
