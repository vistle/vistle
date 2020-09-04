#ifndef VISTLE_LIBSIM_MESH_INFO_H
#define VISTLE_LIBSIM_MESH_INFO_H
#include <vector>
#include <vistle/core/object.h>

namespace vistle {
namespace insitu {
namespace libsim {
struct MeshInfo {
    bool combined = false; //if the mesh is made of multiple smaler meshes
    char* name = nullptr;
    int dim = 0; //2D or 3D
    int numDomains = 0;
    const int* domains = nullptr;
    std::vector<int> handles;
    std::vector< vistle::Object::ptr> grids;
};

}
}
}
#endif