#include "mesh.h"

#include <cassert>
#include <iostream>

using namespace vistle::insitu;

vistle::insitu::MeshObject::MeshObject()
	:m_type(MeshType::Invalid)
{

}

vistle::insitu::MeshObject::MeshObject(const std::string& name, MeshType type, std::array<Array, 3>&& data)
	: m_name(name)
	, m_type(type)
	, m_data(std::move(data))
	, m_interleaved(false)
{
	initGhostToNoGhost();
}

vistle::insitu::MeshObject::MeshObject(const std::string& name, MeshType type, Array&& data, std::array<size_t, 3> dimensions)
	: m_name(name)
	, m_type(type)
	, m_sizes(dimensions)
	, m_data({ std::move(data), nullptr, nullptr })
	, m_interleaved(true)
{
	initGhostToNoGhost();
}

//vistle::insitu::Mesh::Mesh(Mesh&& other) noexcept
//	:m_name(std::move(other.m_name))
//	, m_interleaved(other.m_interleaved)
//	, m_type(other.m_type)
//	, m_min(std::move(other.m_min))
//	, m_max(std::move(other.m_max))
//	, m_data(std::move(other.m_data))
//	, m_sizes(std::move(other.m_sizes))
//	, m_globalBlockIndex(other.m_globalBlockIndex)
//{
//}
//
//Mesh& vistle::insitu::Mesh::operator=(Mesh&& other) noexcept
//{
//	m_name = std::move(other.m_name);
//	m_interleaved = std::move(other.m_interleaved);
//	m_type = std::move(other.m_type);
//	m_min = std::move(other.m_min);
//	m_max = std::move(other.m_max);
//	m_data = std::move(other.m_data);
//	m_sizes = std::move(other.m_sizes);
//	m_globalBlockIndex = std::move(other.m_globalBlockIndex);
//	return *this;
//}



void vistle::insitu::MeshObject::setData(std::array<Array, 3>&& data)
{
	m_data = std::move(data);
	m_interleaved = false;
	if (!m_hasGhost)
	{
		initGhostToNoGhost();
	}
}

void vistle::insitu::MeshObject::setData(Array&& data, std::array<size_t, 3> dimensions)
{
	m_data[0] = std::move(data);
	m_data[1] = Array{};
	m_data[2] = Array{};
	m_interleaved = true;
	m_sizes = dimensions;
	if (!m_hasGhost)
	{
		initGhostToNoGhost();
	}
}

void vistle::insitu::MeshObject::setGostCells(std::array<size_t, 3> min, std::array<size_t, 3> max)
{
	m_min = min;
	m_max = max;
	m_hasGhost = true;
}

size_t MeshObject::numVerts() const {
	if (m_interleaved)
	{
		size_t s = 1;
		for (size_t i = 0; i < 3; i++)
		{
			s *= std::max(m_sizes[0], (size_t)1);
		}
		assert(m_data[0]->size() * static_cast<int>(dim()) == s);
		return s;
	}
	else
	{
		assert(m_data[0]->size() == m_data[1]->size() && m_data[0]->size() == m_data[2]->size());
		return m_data[0]->size();
	}
}

Dimension vistle::insitu::MeshObject::dim() const
{
	if (m_interleaved)
	{
		return m_sizes[2] > 1 ? Dimension::d3D : Dimension::d2D;
	}
	else
	{
		return m_data[2]->size() > 1 ? Dimension::d3D : Dimension::d2D;
	}
}

bool vistle::insitu::MeshObject::isInterleaved() const
{
	return m_interleaved;
}

std::string vistle::insitu::MeshObject::name() const
{
	return m_name;
}

MeshType vistle::insitu::MeshObject::type() const
{
	return m_type;
}

DataMapping vistle::insitu::MeshObject::mapping() const
{
	assert((m_data[0]->mapping() == m_data[1]->mapping() && m_data[0]->mapping() == m_data[2]->mapping()));
	return m_data[0]->mapping();
}

size_t vistle::insitu::MeshObject::size(unsigned int index) const
{
	assert(index < 3);
	if (m_interleaved)
	{
		return m_sizes[index];
	}
	else
	{
		return m_data[index]->size();
	}
}

const Array& vistle::insitu::MeshObject::data(unsigned int index) const
{
	assert((!m_interleaved && index < 3) || (m_interleaved && index == 0));
	return m_data[index];
}

const std::array<size_t, 3>& vistle::insitu::MeshObject::lowerGhostBound() const
{
	if (!checkGhost())
	{
		initGhostToNoGhost();
	}
	return m_min;
}

const std::array<size_t, 3>& vistle::insitu::MeshObject::upperGhostBound() const
{
	return m_max;
}

void vistle::insitu::MeshObject::setGlobalBlockIndex(size_t index)
{
	m_globalBlockIndex = index;
}

size_t vistle::insitu::MeshObject::globalBlockIndex() const
{
	return m_globalBlockIndex;
}

void vistle::insitu::MeshObject::initGhostToNoGhost() const
{
	if (!m_data[0])
	{
		m_min = m_max = { 0,0,0 };
		return;
	}
	if (m_interleaved)
	{
		for (size_t i = 0; i < 3; i++)
		{
			m_min[i] = 0;
			m_max[i] = m_sizes[i] -1;
		}
	}
	else
	{
		for (size_t i = 0; i < 3; i++)
		{
			m_min[i] = 0;
			m_max[i] = m_data[i]->size() - 1;
		}
	}
}

bool vistle::insitu::MeshObject::checkGhost() const 
{
	bool noError = true;
	for (size_t i = 0; i < 3; i++)
	{
		if (m_min[i] > m_max[i])
		{
			std::cerr << "invalid ghost data for mesh " << m_name << ": lower bound index must be lower than upper bound index" << std::endl;
			noError = false;
		}
		if (m_min[i] >= size(i))
		{
			std::cerr << "invalid ghost data for mesh " << m_name << ": lower bound index is outside of data bounds" << std::endl;
			noError = false;
		}
		if (m_max[i] >= size(i))
		{
			std::cerr << "invalid ghost data for mesh " << m_name << ": upper bound index is outside of data bounds" << std::endl;
			noError = false;
		}
	}
	return noError;
}

vistle::insitu::MultiMeshObject::MultiMeshObject(const std::string& name)
	:MeshObject(name, MeshType::Multi)
{

}

MultiMeshObject::Iter vistle::insitu::MultiMeshObject::addMesh(Mesh&& mesh, size_t globalBlockIndex)
{
	mesh->setGlobalBlockIndex(globalBlockIndex);
	m_globalBlockIndices.push_back(globalBlockIndex);
	return m_meshes.emplace(end(), std::move(mesh));
}

MultiMeshObject::Iter vistle::insitu::MultiMeshObject::begin() const
{
	return m_meshes.begin();
}

MultiMeshObject::Iter vistle::insitu::MultiMeshObject::end() const
{
	return m_meshes.end();
}

size_t vistle::insitu::MultiMeshObject::size() const
{
	return m_meshes.size();
}