#ifndef SENSEI_RECTILINEAR_GRID_H
#define SENSEI_RECTILINEAR_GRID_H
#include <insitu/message/SyncShmIDs.h>
#include <insitu/core/dataAdaptor.h>

#include <core/rectilineargrid.h>
namespace vistle {
namespace insitu {
namespace sensei {
vistle::RectilinearGrid::const_ptr makeRectilinearGrid(const Mesh& mesh, message::SyncShmIDs& syncIDs, size_t timestep);
}//sensei
}//insitu
}//vistle

#endif