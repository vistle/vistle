#ifndef VISTLE_SENSEI_INTERFACE_H
#define VISTLE_SENSEI_INTERFACE_H

#include "export.h"
#include "metaData.h"

#include <vistle/core/object.h>
#include <vistle/core/database.h>

namespace vistle {
namespace insitu {
namespace sensei {


class V_SENSEIEXPORT Callbacks {//callbacks to retrieve GridInterfacees and Arrays from sensei
public:
	struct OutputData {
		std::string portName;
		vistle::Object::const_ptr data;
	};
	Callbacks(std::function<vistle::Object::ptr(const std::string&)> getGrid, std::function<vistle::DataBase::ptr(const std::string&)> getVar);
	Callbacks(std::function<std::vector<OutputData>(const MetaData& usedData)> getData);
	std::vector<OutputData> getData(const MetaData& usedData);
	vistle::Object::ptr getGrid(const std::string& name);
	vistle::DataBase::ptr getVar(const std::string& name);
	
private:
	std::function<vistle::Object::ptr(const std::string&)> m_getGrid;
	std::function<vistle::DataBase::ptr(const std::string&)> m_getVar;
	std::function<std::vector<OutputData>(const MetaData& usedData)> m_getData;
};



}//sensei
}//insitu
}//vistle





#endif