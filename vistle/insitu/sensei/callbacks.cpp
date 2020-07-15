#include "callbacks.h"

using namespace vistle::insitu;
using namespace vistle::insitu::sensei;


sensei::Callbacks::Callbacks(std::function<vistle::Object::ptr(const std::string&)> getGrid, std::function<vistle::DataBase::ptr(const std::string&)> getVar)
	:m_getGrid(getGrid)
	,m_getVar(getVar)
{
}

vistle::insitu::sensei::Callbacks::Callbacks(std::function<std::vector<Callbacks::OutputData>(const MetaData& usedData)> getData)
	:m_getData(getData)
{
}

std::vector<Callbacks::OutputData> vistle::insitu::sensei::Callbacks::getData(const MetaData& usedData)
{
	return m_getData(usedData);
}

vistle::Object::ptr sensei::Callbacks::getGrid(const std::string& name)
{
	return m_getGrid(name);
}

vistle::DataBase::ptr sensei::Callbacks::getVar(const std::string& name)
{
	return m_getVar(name);
}


