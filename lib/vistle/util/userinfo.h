#ifndef VISTLE_UTIL_USERINFO_H
#define VISTLE_UTIL_USERINFO_H

#include "export.h"
#include <string>

namespace vistle {

V_UTILEXPORT std::string getLoginName();
V_UTILEXPORT std::string getRealName();
V_UTILEXPORT unsigned long getUserId();

} // namespace vistle
#endif
