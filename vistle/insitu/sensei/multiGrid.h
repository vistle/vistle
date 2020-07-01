#ifndef VISTLE_SENSEI_MULTIMESH_H
#define VISTLE_SENSEI_MULTIMESH_H

#include "gridInterface.h"

namespace vistle {
namespace insitu {
namespace sensei {


class V_SENSEIEXPORT MultiGrid : public GridInterface {
public:
	MultiGrid(const std::string& name) :GridInterface(name) {}
	virtual vistle::Object_const_ptr toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const override;
	virtual size_t numGrids() const override;
	virtual const Grid* getGrid(size_t index) const override;

	void addGrid(Grid&& grid) {
		m_grids.push_back(std::move(grid));

	};

private:
	std::vector<Grid> m_grids;
};
}
}
}

#endif //VISTLE_SENSEI_MULTIMESH_H