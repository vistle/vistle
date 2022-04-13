#ifndef VISTLE_DIRECTORY_H
#define VISTLE_DIRECTORY_H

#include <string>
#include <map>

#include "export.h"

namespace vistle {

namespace directory {

V_UTILEXPORT std::string build_type();
V_UTILEXPORT std::string prefix(int argc, char *argv[]);
V_UTILEXPORT std::string prefix(const std::string &bindir);
V_UTILEXPORT std::string bin(const std::string &prefix);
V_UTILEXPORT std::string module(const std::string &prefix);
V_UTILEXPORT std::string share(const std::string &prefix);
V_UTILEXPORT std::string configHome(); //config directory of the user
V_UTILEXPORT bool setVistleRoot(const std::string &vistleRootDir);

} // namespace directory

} // namespace vistle

#endif
