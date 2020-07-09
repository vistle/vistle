#include "callbacks.h"

using namespace vistle::insitu;
using namespace vistle::insitu::sensei;


sensei::Callbacks::Callbacks(std::function<vistle::Object::ptr(const std::string&)> getGrid, std::function<vistle::DataBase::ptr(const std::string&)> getVar)
	:m_getGrid(getGrid)
	,m_getVar(getVar)
{
}

vistle::Object::ptr sensei::Callbacks::getGrid(const std::string& name)
{
	return m_getGrid(name);
}

vistle::DataBase::ptr sensei::Callbacks::getVar(const std::string& name)
{
	return m_getVar(name);
}


