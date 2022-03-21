#ifndef VISTLE_LIBSIM_STRUCTURED_MESH_H
#define VISTLE_LIBSIM_STRUCTURED_MESH_H

#include <vistle/core/object.h>
#include "SmartHandle.h"
#include <memory>

class visit_handle;
namespace vistle {
class StructuredGrid;
class UnstructuredGrid;
namespace insitu {
namespace message {
class SyncShmIDs;
}
namespace libsim {
struct MeshInfo;
vistle::Object::ptr get(const visit_smart_handle<HandleType::CurvilinearMesh> &meshHandle);

namespace StructuredMesh {
vistle::Object::ptr get(const visit_smart_handle<HandleType::CurvilinearMesh> &meshHandle);
// TODO: consider ghost cells
vistle::Object::ptr getCombinedUnstructured(const MeshInfo &meshInfo, bool vtkFormat = false);

namespace detail {
void getMeshCoord(int currDomain, const char *name, int &ndims, int dims[3], int &coordMode,
                  visit_handle coordHandles[4]);
void fillMeshCoords(int coordMode, visit_handle coordHandles[4], size_t numVertices,
                    std::array<vistle::Scalar *, 3> &gridCoords, int dim);
void separateFill(visit_handle coordHandles[4], int numCoords, std::array<vistle::Scalar *, 3> &meshCoords, int dim);
void interleavedFill(visit_handle coordHandle, int numCoords, const std::array<vistle::Scalar *, 3> &meshCoords,
                     int dim);
void addGhost(const visit_handle &meshHandle, std::shared_ptr<vistle::StructuredGrid> mesh);

} // namespace detail
} // namespace StructuredMesh
} // namespace libsim
} // namespace insitu
} // namespace vistle

#endif // !VISTLE_LIBSIM_STRUCTURED_MESH_H
