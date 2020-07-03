#include "multiGrid.h"

vistle::Object_const_ptr vistle::insitu::sensei::MultiGrid::toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const
{
	return nullptr;
}

size_t vistle::insitu::sensei::MultiGrid::numGrids() const
{
	return m_grids.size();
}

const vistle::insitu::sensei::Grid* vistle::insitu::sensei::MultiGrid::getGrid(size_t index) const
{
	if (index >= m_grids.size())
	{
		return nullptr;
	}
	return &m_grids[index];
}


