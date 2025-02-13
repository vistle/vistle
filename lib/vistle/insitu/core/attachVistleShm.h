#ifndef VISTLE_INSITU_CORE_ATTACHVISTLESHM_H
#define VISTLE_INSITU_CORE_ATTACHVISTLESHM_H

#include "export.h"
#include <string>
namespace vistle {
namespace insitu {
void V_INSITUCOREEXPORT attachShm(const std::string &name, int id, int rank);
void V_INSITUCOREEXPORT detachShm();
} // namespace insitu
} // namespace vistle

#endif
