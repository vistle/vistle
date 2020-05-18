#include "structuredGrid.h"		
#include <insitu/core/transformArray.h>
using namespace vistle::insitu::sensei;
using namespace vistle::insitu;
vistle::StructuredGrid::const_ptr vistle::insitu::sensei::makeStructuredGrid(const Mesh& mesh, message::SyncShmIDs& syncIDs, size_t timestep)
{
	assert(mesh.type == vistle::Object::STRUCTUREDGRID);

	vistle::StructuredGrid::ptr grid;
	if (mesh.interleaved)
	{
		grid = syncIDs.createVistleObject<vistle::StructuredGrid>(mesh.sizes[0], mesh.sizes[1], mesh.sizes[2]);
		transformInterleavedArray(mesh.data[0].data, std::array<vistle::Scalar*, 3>{ grid->x().data(), grid->y().data(), grid->z().data() }, mesh.data[0].size, mesh.data[0].dataType, static_cast<int>(mesh.dim)); //not tested
	}
	else {
		grid = syncIDs.createVistleObject<vistle::StructuredGrid>(mesh.data[0].size, mesh.data[1].size, mesh.data[2].size);


		transformArray(mesh.data[0], grid->x().begin());
		transformArray(mesh.data[1], grid->y().begin());
		transformArray(mesh.data[2], grid->z().begin());

		for (size_t i = 0; i < 3; i++) {
			assert(mesh.min[i] >= 0);
			int numTop = mesh.data[i].size - 1 - mesh.max[i];
			assert(numTop >= 0);
			grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Bottom, mesh.min[i]);
			grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Top, numTop);
		}

	}
	grid->setBlock(mesh.blockNumber);
	grid->setTimestep(timestep);
	return grid;
}
