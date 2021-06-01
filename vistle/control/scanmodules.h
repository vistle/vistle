#ifndef VISTLE_SCANMODULES_H
#define VISTLE_SCANMODULES_H

#include <vistle/core/availablemodule.h>

#include "export.h"

namespace vistle {

V_HUBEXPORT bool scanModules(const std::string &directory, int hub,
                              AvailableMap &available);

}
#endif
