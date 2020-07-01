#include "structuredGridBase.h"
#include <iostream>
#include <cassert>

vistle::insitu::sensei::StructuredGridBase::StructuredGridBase(std::array<Array, 3>&& data, const std::array<size_t, 3>& dimensions)
	: m_sizes(dimensions)
	, m_data(std::move(data))
{
	assert((m_data[0] && m_sizes[0] > 1) || !m_data[0] && m_sizes[0] <= 1);
	assert((m_data[1] && m_sizes[1] > 1) || !m_data[1] && m_sizes[1] <= 1);
	assert((m_data[2] && m_sizes[2] > 1) || !m_data[2] && m_sizes[2] <= 1);
}

vistle::insitu::sensei::StructuredGridBase::StructuredGridBase(std::array<Array, 3>&& data)
	: m_data(std::move(data))
{
	for (size_t i = 0; i < 3; i++)
	{
		if (m_data[i])
		{
			m_sizes[i] = m_data[i].size();
		}
		else
		{
			m_sizes[i] = 1;
		}
	}
}

vistle::insitu::sensei::StructuredGridBase::StructuredGridBase(vistle::insitu::Array&& data, const std::array<size_t, 3> dimensions)
	:StructuredGridBase({ std::move(data), Array{}, Array{} }, dimensions)
{
	m_interleaved = true;
}

void vistle::insitu::sensei::StructuredGridBase::setGhostCells(const std::array<size_t, 3> &min, const std::array<size_t, 3> &max)
{
	m_min = min;
	m_max = max;
	m_hasGhost = true;
}

void vistle::insitu::sensei::StructuredGridBase::setGlobalBlockIndex(size_t index)
{
	m_globalBlockIndex = index;
}

unsigned int vistle::insitu::sensei::StructuredGridBase::dim() const
{
	unsigned int i = 0;
	while (m_sizes[i] > 1)
	{
		++i;
	}
	return i;
}

void vistle::insitu::sensei::StructuredGridBase::resetGhostToNoGhost() const
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
			m_max[i] = m_sizes[i] - 1;
		}
	}
	else
	{
		for (size_t i = 0; i < 3; i++)
		{
			m_min[i] = 0;
			m_max[i] = m_data[i].size() - 1;
		}
	}
}

bool vistle::insitu::sensei::StructuredGridBase::checkGhost() const
{
	bool noError = true;
	for (size_t i = 0; i < 3; i++)
	{
		if (m_min[i] > m_max[i])
		{
			std::cerr << "invalid ghost data for mesh: lower bound index must be lower than upper bound index" << std::endl;
			noError = false;
		}
		if (m_min[i] >= m_sizes[i])
		{
			std::cerr << "invalid ghost data for mesh: lower bound index is outside of data bounds" << std::endl;
			noError = false;
		}
		if (m_max[i] >= m_sizes[i])
		{
			std::cerr << "invalid ghost data for mesh: upper bound index is outside of data bounds" << std::endl;
			noError = false;
		}
	}
	return noError;
}