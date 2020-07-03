#ifndef SENSEI_RECTILINEAR_GRID_H
#define SENSEI_RECTILINEAR_GRID_H

#include <insitu/core/mesh.h>

#include "gridInterface.h"
#include "structuredGridBase.h"
namespace vistle {
namespace insitu {
namespace sensei {


class V_SENSEIEXPORT RectilinearGrid : public GridInterface, private StructuredGridBase {
public:
	virtual vistle::Object_const_ptr toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const override;

	RectilinearGrid(const std::string& name, std::array<vistle::insitu::Array, 3>&& data); //create Mesh with separate data for each dimension
	RectilinearGrid(const std::string& name, vistle::insitu::Array&& data, const std::array<size_t, 3> dimensions); //create Mesh from interleaved data

};

}//sensei
}//insitu
}//vistle

#endif