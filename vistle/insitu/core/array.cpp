#include "array.h"

#include <insitu/core/callFunctionWithVoidToTypeCast.h>

#include<cassert>

using namespace vistle::insitu;

Array::Array(const std::string& name)
	:m_name(name)
{
}

Array::Array(Array &&other)
	: m_name(other.m_name)
	, m_size(other.m_size)
	, m_dataType(other.m_dataType)
	, m_mapping(other.m_mapping)
	, m_owner(other.m_owner)
	, m_data(other.m_data)
{
	other.m_data = nullptr;
	other.m_size = 0;
	other.m_owner = Owner::Unknown;
	other.m_name = "";
}
Array::Array(const std::string& name, void* data, size_t size, DataType dataType, DataMapping mapping, Owner owner)
	: m_name(name)
	, m_size(size)
	, m_dataType(dataType)
	, m_mapping(mapping)
	, m_owner(owner)
	, m_data(data)
{
}

Array& Array::operator=(Array&& other)
{
	m_name = other.m_name;
	m_size = other.m_size;
	m_dataType = other.m_dataType;
	m_mapping = other.m_mapping;
	m_owner = other.m_owner;
	m_data = other.m_data;

	other.m_data = nullptr;
	other.m_size = 0;
	other.m_owner = Owner::Unknown;
	other.m_name = "";
	return *this;
}

void* vistle::insitu::Array::data()
{
	assert((m_dataType == DataType::INVALID) && !m_data || (m_dataType != DataType::INVALID && m_data));
	return m_data;
}

const void* vistle::insitu::Array::data() const
{
	assert((m_dataType == DataType::INVALID) && !m_data || (m_dataType != DataType::INVALID && m_data));
	return m_data;
}

const std::string& Array::name() const
{
	return m_name;
}


const size_t Array::size() const
{
	return m_size;
}

const DataType Array::dataType() const
{
	return m_dataType;
}

const DataMapping Array::mapping() const
{
	return m_mapping;
}

const Owner Array::owner() const
{
	return m_owner;
}

vistle::insitu::Array::operator bool() const
{
	return m_data;
}


template<typename T>
struct DataDeleter {
	void operator()(const T* data, size_t s)//s is only to fullfil requirements for detail::callFunctionWithVoidToTypeCast
	{
		delete[] data;
	}
};


vistle::insitu::Array::~Array()
{
	
	if (m_owner == Owner::Vistle)
	{
		if (m_dataType == DataType::ARRAY)
		{
			DataDeleter<Array>()(dataAs<Array>(), m_size);
		}
		else
		{
			detail::callFunctionWithVoidToTypeCast<void, DataDeleter>(m_data, m_dataType, m_size);
		}
	}
}
