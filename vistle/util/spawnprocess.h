#ifndef SPAWNPROCESS_H
#define SPAWNPROCESS_H

#include <string>
#include <vector>
#include "export.h"

namespace vistle {

#ifdef _WIN32
#else
typedef long process_handle;
#endif

V_UTILEXPORT process_handle spawn_process(const std::string &executable, const std::vector<std::string> args);
V_UTILEXPORT process_handle try_wait();

} // namespace vistle
#endif
