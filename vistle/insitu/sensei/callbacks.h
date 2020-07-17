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
	struct V_SENSEIEXPORT OutputData {
		OutputData() = default;
		OutputData(const std::string& gridName, vistle::Object::ptr obj);
		OutputData(const std::string& gridName, const std::string& varName, vistle::Object::ptr obj);
		const std::string& portName() const;
		vistle::Object::ptr object() const;
		operator bool() const;
	private:
		std::string m_portName;
		vistle::Object::ptr m_obj;
	};
	Callbacks(std::function<std::vector<OutputData>(const MetaData& usedData)> getData);
	std::vector<OutputData> getData(const MetaData& usedData);

	
private:
	std::function<vistle::DataBase::ptr(const std::string&)> m_getVar;
	std::function<std::vector<OutputData>(const MetaData& usedData)> m_getData;
};



}//sensei
}//insitu
}//vistle





#endif