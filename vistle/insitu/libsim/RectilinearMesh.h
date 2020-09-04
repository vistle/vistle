#ifndef VISTLE_LIBSIM_RECTILINEAR_MESH_H
#define VISTLE_LIBSIM_RECTILINEAR_MESH_H

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
	std::shared_ptr<vistle::RectilinearGrid> get(const visit_handle &meshHandle, message::SyncShmIDs& creator);

	//todo: consider ghost cells
	std::shared_ptr<vistle::UnstructuredGrid> getCombinedUnstructured(const MeshInfo& meshInfo, message::SyncShmIDs& creator, bool vtkFormat = false);
namespace detail {
	static std::shared_ptr<vistle::RectilinearGrid> makeVistleMesh(void* data[3], int sizes[3], int dataType[3], message::SyncShmIDs& creator);
	static void addGhost(const visit_handle& meshHandle, std::shared_ptr<vistle::RectilinearGrid> mesh);
	void getMeshFromSim(const visit_handle& meshHandle, int owner[3], int dataType[3], int nTuples[], void* data[]);
}//detail
}//RectilinearMesh
}//libsim
}//insitu
}//vistle


#endif // !VISTLE_LIBSIM_RECTILINEAR_MESH_H
