#include "rectilinerGrid.h"

#include <insitu/core/transformArray.h>
#include <insitu/message/SyncShmIDs.h>

#include <core/rectilineargrid.h>

using namespace vistle::insitu::sensei;
using namespace vistle::insitu;

vistle::Object_const_ptr vistle::insitu::sensei::RectilinearGrid::toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const
{
	vistle::RectilinearGrid::ptr grid;
	if (m_interleaved)
	{
		grid = syncIDs.createVistleObject<vistle::RectilinearGrid>(m_sizes[0], m_sizes[1], m_sizes[2]);
		transformInterleavedArray(m_data[0].data(), std::array<vistle::Scalar*, 3>{ grid->coords(0).data(), grid->coords(1).data(), grid->coords(2).data() }, m_data[0].size(), m_data[0].dataType(), dim()); //not tested
	}
	else {
		grid = syncIDs.createVistleObject<vistle::RectilinearGrid>(m_data[0].size(), m_data[1].size(), m_data[2].size());
		unsigned int dim = 0;
		while (m_data[dim])
		{
			transformArray(m_data[dim].data(), grid->coords(dim).begin(), m_data[dim].size(), m_data[dim].dataType());
			++dim;
		}

		if (!checkGhost())
		{
			std::cerr << "grid " << name() << ": ghost cells invalid, resetting ghost data" << std::endl;
			resetGhostToNoGhost();
		}
		for (size_t i = 0; i < dim; i++) {
			int numTop = m_data[i].size() - 1 -m_max[i];

			grid->setNumGhostLayers(i, vistle::RectilinearGrid::GhostLayerPosition::Bottom, m_min[i]);
			grid->setNumGhostLayers(i, vistle::RectilinearGrid::GhostLayerPosition::Top, numTop);
		}

	}
	grid->setBlock(m_globalBlockIndex);
	grid->setTimestep(timestep);
	return grid;
}

vistle::insitu::sensei::RectilinearGrid::RectilinearGrid(const std::string& name, std::array<Array, 3>&& data)
	: GridInterface(name)
	,StructuredGridBase(std::move(data))
{

}

vistle::insitu::sensei::RectilinearGrid::RectilinearGrid(const std::string& name, vistle::insitu::Array&& data, const std::array<size_t, 3> dimensions)
	:GridInterface(name)
	,StructuredGridBase(std::move(data), dimensions)
{
}

