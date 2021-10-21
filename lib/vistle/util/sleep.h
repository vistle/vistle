#ifndef VISTLE_SLEEP_H
#define VISTLE_SLEEP_H

#include "export.h"

namespace vistle {

V_UTILEXPORT bool adaptive_wait(bool work, const void *client = nullptr);

} // namespace vistle
#endif
