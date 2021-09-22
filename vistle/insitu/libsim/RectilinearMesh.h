#ifndef VISTLE_LIBSIM_RECTILINEAR_MESH_H
#define VISTLE_LIBSIM_RECTILINEAR_MESH_H

#include "ArrayStruct.h"

#include <vistle/core/object.h>

#include <memory>

class visit_handle;
namespace vistle {
class RectilinearGrid;
class UnstructuredGrid;
namespace insitu {
namespace message {
class SyncShmIDs;
}
namespace libsim {
struct MeshInfo;

std::shared_ptr<vistle::Object> get(const visit_smart_handle<HandleType::RectilinearMesh> &meshHandle,
                                    message::SyncShmIDs &creator);

namespace RectilinearMesh {

// Inherited via GetVistleObjectInterface
std::shared_ptr<vistle::Object> get(const visit_handle &meshHandle, message::SyncShmIDs &creator);

//todo: consider ghost cells
std::shared_ptr<vistle::Object> getCombinedUnstructured(const MeshInfo &meshInfo, message::SyncShmIDs &creator,
                                                        bool vtkFormat = false);
namespace detail {
std::shared_ptr<vistle::RectilinearGrid> makeVistleMesh(const std::array<Array<HandleType::Coords>, 3> &meshData,
                                                        message::SyncShmIDs &creator);
void addGhost(const visit_handle &meshHandle, std::shared_ptr<vistle::RectilinearGrid> mesh);
std::array<Array<HandleType::Coords>, 3>
getMeshFromSim(const visit_smart_handle<HandleType::RectilinearMesh> &meshHandle);
} // namespace detail
} // namespace RectilinearMesh
} // namespace libsim
} // namespace insitu
} // namespace vistle


#endif // !VISTLE_LIBSIM_RECTILINEAR_MESH_H
