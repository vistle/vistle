#ifndef VISTLE_LIBSIM_MESH_INFO_H
#define VISTLE_LIBSIM_MESH_INFO_H

#include <vistle/insitu/libsim/libsimInterface/VisItDataTypes.h>

#include <vistle/core/object.h>

#include <vector>

namespace vistle {
namespace insitu {
namespace libsim {
struct MeshInfo {
    bool combined = false; //if the mesh is made of multiple smaler meshes
    const char* name = nullptr;
    int dim = 0; //2D or 3D
    int numDomains = 0;
    const int* domains = nullptr;
    std::vector<int> handles;
    std::vector< vistle::Object::ptr> grids;
    VisIt_MeshType type = VISIT_MESHTYPE_UNKNOWN;
};

}
}
}
#endif