#ifndef VISTLE_SCAN_SCRIPTS_H
#define VISTLE_SCAN_SCRIPTS_H
#include <vector>
#include <string>
namespace vistle {
//search in user config dir and in dirs/files found in VISTLE_PRELOAD_SCRIPTS
std::vector<std::string> scanStartupScripts();
} // namespace vistle


#endif // VISTLE_SCAN_SCRIPTS_H
