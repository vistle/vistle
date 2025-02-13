#ifndef VISTLE_INSITU_LIBSIM_MESHINFO_H
#define VISTLE_INSITU_LIBSIM_MESHINFO_H

#include "ArrayStruct.h"

#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

#include <vistle/core/object.h>

#include <vector>

namespace vistle {
namespace insitu {
namespace libsim {
struct MeshInfo {
    bool combined = false; //if the mesh is made of multiple smaller meshes
    std::string name;
    int dim = 0; //2D or 3D
    visit_smart_handle<HandleType::DomainList> domainHandle;
    Array<HandleType::Coords> domains;
    //std::vector<int> handles;
    std::vector<vistle::Object::ptr> grids;
    VisIt_MeshType type = VISIT_MESHTYPE_UNKNOWN;
};

} // namespace libsim
} // namespace insitu
} // namespace vistle
#endif
