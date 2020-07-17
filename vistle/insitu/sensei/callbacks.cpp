#include "callbacks.h"

using namespace vistle::insitu;
using namespace vistle::insitu::sensei;


vistle::insitu::sensei::Callbacks::Callbacks(std::function<std::vector<OutputData>(const MetaData& usedData)> getData)
	:m_getData(getData)
{
}

std::vector<Callbacks::OutputData> vistle::insitu::sensei::Callbacks::getData(const MetaData& usedData)
{
	return m_getData(usedData);
}


vistle::insitu::sensei::Callbacks::OutputData::OutputData(const std::string& gridName, vistle::Object::ptr obj)
	: m_portName(gridName)
	, m_obj(obj)
{
}

vistle::insitu::sensei::Callbacks::OutputData::OutputData(const std::string& gridName, const std::string& varName, vistle::Object::ptr obj)
	: m_portName(gridName + "_" + varName)
	, m_obj(obj)
{
}

const std::string& vistle::insitu::sensei::Callbacks::OutputData::portName() const
{
	return m_portName;
}

vistle::Object::ptr vistle::insitu::sensei::Callbacks::OutputData::object() const
{
	return m_obj;
}

vistle::insitu::sensei::Callbacks::OutputData::operator bool() const
{
	return m_obj != nullptr;
}


