#ifndef VISTLE_LIBSIM_VARIABLE_INFO_H
#define VISTLE_LIBSIM_VARIABLE_INFO_H

#include "MeshInfo.h"

#include <vistle/core/database.h>
#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

#include <vector>

namespace vistle {
namespace insitu {
namespace libsim {
struct VariableInfo {
    VariableInfo(const MeshInfo &mesh): meshInfo(mesh){};
    std::string name;
    vistle::DataBase::Mapping mapping = vistle::DataBase::Mapping::Unspecified;
    const MeshInfo &meshInfo;
    std::vector<vistle::Object::ptr> variables;
};

} // namespace libsim
} // namespace insitu
} // namespace vistle
#endif
