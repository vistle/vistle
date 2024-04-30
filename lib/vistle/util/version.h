#ifndef VISTLE_VERSION_H
#define VISTLE_VERSION_H

#include "export.h"

#include <string>

namespace vistle {
namespace version {

std::string V_UTILEXPORT tag(); // newest git tag contained by HEAD
std::string V_UTILEXPORT hash(); // git hash of HEAD
std::string V_UTILEXPORT string(); // tag() + "-" + hash()

std::string V_UTILEXPORT flags(); // compile-time configuration
std::string V_UTILEXPORT arch(); // cpu architecture
std::string V_UTILEXPORT os(); // operating system

std::string V_UTILEXPORT copyright();
std::string V_UTILEXPORT license();

std::string V_UTILEXPORT homepage();
std::string V_UTILEXPORT github();

std::string V_UTILEXPORT banner(); // all version info formatted as multiple lines

} // namespace version
} // namespace vistle
#endif
