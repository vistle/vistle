#ifndef SENSEI_UNSTRUCTURED_GRID_H
#define SENSEI_UNSTRUCTURED_GRID_H
#include <insitu/message/SyncShmIDs.h>
#include <insitu/core/dataAdaptor.h>
#include <core/unstr.h>
namespace vistle {
namespace insitu {
namespace sensei {
	vistle::UnstructuredGrid::const_ptr makeUnstructuredGrid(const UnstructuredMesh& mesh, message::SyncShmIDs& syncIDs, size_t timestep);


}//sensei
}//insitu
}//vistle

#endif			
			
