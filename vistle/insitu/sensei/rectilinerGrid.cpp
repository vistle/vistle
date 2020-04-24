#include "rectilinerGrid.h"
#include <insitu/core/transformArray.h>
#include <core/structuredgrid.h>

using namespace insitu;

vistle::Object::const_ptr insitu::makeRectilinearGrid(const Mesh& mesh, message::SyncShmIDs& syncIDs)
{
	assert(mesh.type == vistle::Object::RECTILINEARGRID);
	if (mesh.interleaved)
	{
		auto grid = syncIDs.createVistleObject<vistle::RectilinearGrid>(mesh.sizes[0], mesh.sizes[1], mesh.sizes[2]);
		transformInterleavedArray(mesh.data[0].data, std::array<vistle::Scalar* ,3>{ grid->coords(0).data(), grid->coords(1).data(), grid->coords(2).data() }, mesh.data[0].size, mesh.data[0].dataType, static_cast<int>(mesh.dim)); //not tested
	}
	else {
		auto grid = syncIDs.createVistleObject<vistle::RectilinearGrid>(mesh.data[0].size, mesh.data[1].size, mesh.data[2].size);
		
		
		for (size_t i = 0; i < static_cast<int>(mesh.dim); i++)
		{
			transformArray(mesh.data[i].data, grid->coords(i).begin(), mesh.data[i].size, mesh.data[i].dataType);
		}

		for (size_t i = 0; i < 3; i++) {
			assert(mesh.min[i] >= 0);
			int numTop = mesh.data[i].size - 1 - mesh.max[i];
			assert(numTop >= 0);
			grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Bottom, mesh.min[i]);
			grid->setNumGhostLayers(i, vistle::StructuredGrid::GhostLayerPosition::Top, numTop);
		}
		return grid;
	}

	


}
