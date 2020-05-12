#include "unstructuredGrid.h"
#include <insitu/core/transformArray.h>
#include "exeption.h"

using namespace insitu;
vistle::UnstructuredGrid::const_ptr insitu::makeUnstructuredGrid(const UnstructuredMesh& mesh, message::SyncShmIDs& syncIDs, size_t timestep)
{
	auto grid = syncIDs.createVistleObject<vistle::UnstructuredGrid>(mesh.el.size, mesh.cl.size, mesh.numVerts());
	
	vistle::UnstructuredGrid::ptr g = std::make_shared<vistle::UnstructuredGrid>(mesh.el.size, mesh.cl.size, mesh.numVerts());
	if (!mesh.elToVistle && !mesh.elToVistle(grid->el().data()))
	{
		throw sensei::Exeption() << "makeUnstructuredGrid failed to convert type list";
	}
	if (!mesh.tlToVistle && !mesh.tlToVistle(grid->tl().data()))
	{
		throw sensei::Exeption() << "makeUnstructuredGrid failed to convert type list";
	}
	if (!mesh.clToVistle && !mesh.clToVistle(grid->cl().data()))
	{
		throw sensei::Exeption() << "makeUnstructuredGrid failed to convert type list";
	}
	if (mesh.interleaved)
	{
		std::array<vistle::Scalar*, 3> d{ grid->x().data(), grid->y().data(), grid->z().data()};
		transformInterleavedArray(mesh.data[0], d, static_cast<int>(mesh.dim));
	}
	else
	{
		transformArray(mesh.data[0], grid->x().data());
		transformArray(mesh.data[1], grid->y().data());
		transformArray(mesh.data[2], grid->z().data());
	}
	grid->setBlock(mesh.blockNumber);
	grid->setTimestep(timestep);
	return grid;
}
