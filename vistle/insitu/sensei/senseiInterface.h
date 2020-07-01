#ifndef VISTLE_SENSEI_INTERFACE_H
#define VISTLE_SENSEI_INTERFACE_H

#include "export.h"
#include "gridInterface.h"

#include <insitu/core/moduleInfo.h>
#include <insitu/core/metaData.h>
#include <insitu/core/dataType.h>
#include <insitu/core/array.h>

#include <string>
#include <memory>
#include <functional>


namespace vistle {
namespace insitu {
namespace sensei {


class V_SENSEIEXPORT Callbacks {//callbacks to retrieve GridInterfacees and Arrays from sensei
public:
	Callbacks(std::function<Grid(const std::string&)> getGrid, std::function<Array(const std::string&)> getVar);
	Grid getGrid(const std::string& name);
	Array getVar(const std::string& name);
	
private:
	std::function<Grid(const std::string&)> m_getGrid;
	std::function<Array(const std::string&)>m_getVar;
};

class V_SENSEIEXPORT SenseiInterface {
public:

	virtual bool Execute(size_t timestep) = 0;
	virtual bool Finalize() = 0;

	virtual bool processTimestep(size_t timestep) = 0; //true if we have sth to do for the given timestep
	virtual ~SenseiInterface();
};

std::unique_ptr<SenseiInterface> V_SENSEIEXPORT createSenseiInterface(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs); 

}//sensei
}//insitu
}//vistle





#endif