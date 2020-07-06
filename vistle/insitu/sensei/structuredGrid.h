#ifndef SENSEI_STRUCTURED_GRID_H
#define SENSEI_STRUCTURED_GRID_H

#include "structuredGridBase.h"
#include "gridInterface.h"

#include <insitu/core/array.h>

namespace vistle {
namespace insitu {
namespace sensei{
class V_SENSEIEXPORT StructuredGrid : public GridInterface, public StructuredGridBase {
public:
	virtual vistle::Object_const_ptr toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const override;

	StructuredGrid(const std::string& name, std::array<vistle::insitu::Array, 3>&& data, const std::array<size_t, 3> &dimensions); //create Mesh with separate data for each dimension
	StructuredGrid(const std::string& name, vistle::insitu::Array&& data, const std::array<size_t, 3> dimensions); //create Mesh from interleaved data

	
};


}//sensei
}//insitu
}//vistle

#endif