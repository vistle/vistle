#include "structuredGrid.h"		

#include <insitu/core/transformArray.h>
#include <insitu/core/mesh.h>
#include <insitu/message/SyncShmIDs.h>

#include <core/structuredgrid.h>

#include <cassert>

using namespace vistle::insitu::sensei;
using namespace vistle::insitu;


vistle::Object_const_ptr vistle::insitu::sensei::StructuredGrid::toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const
{
	vistle::StructuredGrid::ptr grid = syncIDs.createVistleObject<vistle::StructuredGrid>(m_sizes[0], m_sizes[1], m_sizes[2]);
	if (m_interleaved)
	{
		transformInterleavedArray(m_data[0].data(), std::array<vistle::Scalar*, 3>{ grid->x().data(), grid->y().data(), grid->z().data() }, m_data[0].size(), m_data[0].dataType(), m_sizes[2] < 2? 2:3); //not tested
	}
	else {
		transformArray(m_data[0], grid->x().begin());
		transformArray(m_data[1], grid->y().begin());
		transformArray(m_data[2], grid->z().begin());
		if (!checkGhost())
		{
			std::cerr << "grid " << name() << ": ghost cells invalid, resetting ghost data" << std::endl;
			resetGhostToNoGhost();
		}
		for (size_t i = 0; i < 3; i++) {
			int numTop = m_data[i].size() - 1 - m_max[i];
			grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Bottom, m_min[i]);
			grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Top, numTop);
		}

	}
	grid->setBlock(m_globalBlockIndex);
	grid->setTimestep(timestep);
	return grid;
}

vistle::insitu::sensei::StructuredGrid::StructuredGrid(const std::string& name, std::array<Array, 3>&& data, const std::array<size_t, 3>& dimensions)
	:GridInterface(name)
	, StructuredGridBase(std::move(data), dimensions)

{
	resetGhostToNoGhost();
}

vistle::insitu::sensei::StructuredGrid::StructuredGrid(const std::string& name, vistle::insitu::Array&& data, std::array<size_t, 3> dimensions)
	:StructuredGrid(name, { std::move(data), Array{}, Array{} }, dimensions)
{
	m_interleaved = true;
	resetGhostToNoGhost();
}





