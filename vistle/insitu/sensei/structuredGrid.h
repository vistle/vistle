#ifndef SENSEI_STRUCTURED_GRID_H
#define SENSEI_STRUCTURED_GRID_H
#include <insitu/message/SyncShmIDs.h>
#include <insitu/core/dataAdaptor.h>
#include <core/structuredgrid.h>

namespace insitu {
	vistle::StructuredGrid::const_ptr makeStructuredGrid(const Mesh& mesh, message::SyncShmIDs& syncIDs, size_t timestep);


}

#endif