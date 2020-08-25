#ifndef VISLTE_PREPARE_SHM_NAME_H
#define VISLTE_PREPARE_SHM_NAME_H
#include "export.h"
#include <string>
namespace vistle {
namespace insitu {
std::string V_INSITUCOREEXPORT cutRankSuffix(const std::string &shmName);

} // namespace insitu
} // namespace vistle

#endif // !VISLTE_PREPARE_SHM_NAME_H
