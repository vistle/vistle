#ifndef SPAWNPROCESS_H
#define SPAWNPROCESS_H

#include <string>
#include <vector>
#include "export.h"
#include "sysdep.h"

namespace vistle {

#ifdef _WIN32
typedef intptr_t process_handle;
#else
typedef long process_handle;
#endif

V_UTILEXPORT process_handle spawn_process(const std::string &executable, const std::vector<std::string> args);
V_UTILEXPORT bool kill_process(process_handle pid);
V_UTILEXPORT process_handle try_wait(int *status=nullptr);
V_UTILEXPORT process_handle try_wait(process_handle pid, int *status=nullptr);

} // namespace vistle
#endif
