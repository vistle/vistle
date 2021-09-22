#ifndef TOOLS_H
#define TOOLS_H

#include <string>

#include "export.h"

namespace vistle {

V_UTILEXPORT std::string backtrace();
V_UTILEXPORT bool attach_debugger();

V_UTILEXPORT bool parentProcessDied();

} // namespace vistle

#endif
