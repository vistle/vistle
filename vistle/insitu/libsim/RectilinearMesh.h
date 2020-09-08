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
namespace RectilinearMesh {

	// Inherited via GetVistleObjectInterface
	std::shared_ptr<vistle::Object> get(const visit_handle &meshHandle, message::SyncShmIDs& creator);

	//todo: consider ghost cells
	std::shared_ptr<vistle::Object> getCombinedUnstructured(const MeshInfo& meshInfo, message::SyncShmIDs& creator, bool vtkFormat = false);
namespace detail {
	static std::shared_ptr<vistle::RectilinearGrid> makeVistleMesh(const std::array<Array, 3>& meshData, message::SyncShmIDs& creator);
	static void addGhost(const visit_handle& meshHandle, std::shared_ptr<vistle::RectilinearGrid> mesh);
	std::array<Array, 3> getMeshFromSim(const visit_handle& meshHandle);
}//detail
}//RectilinearMesh
}//libsim
}//insitu
}//vistle


#endif // !VISTLE_LIBSIM_RECTILINEAR_MESH_H
