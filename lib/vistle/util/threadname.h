#ifndef VISTLE_UTIL_THREADNAME_H
#define VISTLE_UTIL_THREADNAME_H

#include "export.h"
#include <string>

namespace vistle {

V_UTILEXPORT bool setThreadName(std::string name);
V_UTILEXPORT std::string getThreadName();
} // namespace vistle
#endif
