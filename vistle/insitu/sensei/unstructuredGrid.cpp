#include "exeption.h"
#include "unstructuredGrid.h"

#include <insitu/core/mesh.h>
#include <insitu/core/transformArray.h>
#include <insitu/message/SyncShmIDs.h>

#include <core/unstr.h>

using namespace vistle::insitu::sensei;
using namespace vistle::insitu;


vistle::Object_const_ptr vistle::insitu::sensei::UnstructuredMesh::toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const
{


	auto grid = syncIDs.createVistleObject<vistle::UnstructuredGrid>(el.size(), cl.size(), numVerts());

	if (!elToVistle || !elToVistle(grid->el().data()))
	{
		throw vistle::insitu::sensei::Exeption() << "makeUnstructuredGrid failed to convert type list";
	}
	if (!tlToVistle || !tlToVistle(grid->tl().data()))
	{
		throw vistle::insitu::sensei::Exeption() << "makeUnstructuredGrid failed to convert type list";
	}
	if (!clToVistle || !clToVistle(grid->cl().data()))
	{
		throw vistle::insitu::sensei::Exeption() << "makeUnstructuredGrid failed to convert type list";
	}
	if (m_interleaved)
	{
		std::array<vistle::Scalar*, 3> d{ grid->x().data(), grid->y().data(), grid->z().data() };
		transformInterleavedArray(m_data[0], d, dim());
	}
	else
	{
		transformArray(m_data[0], grid->x().data());
		transformArray(m_data[1], grid->y().data());
		transformArray(m_data[2], grid->z().data());
	}
	grid->setBlock(m_globalBlockIndex);
	grid->setTimestep(timestep);
	return grid;
}

size_t vistle::insitu::sensei::UnstructuredMesh::numVerts() const
{
	if (m_interleaved)
	{
		size_t s = 1;
		for (size_t i = 0; i < 3; i++)
		{
			s *= std::max(m_sizes[0], (size_t)1);
		}
		assert(m_data[0].size() * static_cast<int>(dim()) == s);
		return s;
	}
	else
	{
		assert(m_data[0].size() == m_data[1].size() && m_data[0].size() == m_data[2].size());
		return m_data[0].size();
	}
}
