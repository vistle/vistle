#ifndef VISTLE_SENSEI_INTERFACE_H
#define VISTLE_SENSEI_INTERFACE_H

#include "export.h"

#include <core/object.h>
#include <core/database.h>

namespace vistle {
namespace insitu {
namespace sensei {


class V_SENSEIEXPORT Callbacks {//callbacks to retrieve GridInterfacees and Arrays from sensei
public:
	Callbacks(std::function<vistle::Object::ptr(const std::string&)> getGrid, std::function<vistle::DataBase::ptr(const std::string&)> getVar);
	vistle::Object::ptr getGrid(const std::string& name);
	vistle::DataBase::ptr getVar(const std::string& name);
	
private:
	std::function<vistle::Object::ptr(const std::string&)> m_getGrid;
	std::function<vistle::DataBase::ptr(const std::string&)> m_getVar;
};



}//sensei
}//insitu
}//vistle





#endif