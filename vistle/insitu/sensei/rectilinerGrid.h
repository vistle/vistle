#ifndef SENSEI_RECTILINEAR_GRID_H
#define SENSEI_RECTILINEAR_GRID_H
#include <insitu/message/SyncShmIDs.h>
#include <core/rectilineargrid.h>
#include <insitu/core/dataAdaptor.h>

namespace insitu {
	vistle::Object::const_ptr makeRectilinearGrid(const Mesh& mesh, message::SyncShmIDs& syncIDs);


}


#endif