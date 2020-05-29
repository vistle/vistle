#ifndef VISTLE_SENSEI_INTERFACE_H
#define VISTLE_SENSEI_INTERFACE_H

#include "export.h"
#include <insitu/core/dataAdaptor.h>
#include <insitu/core/moduleInfo.h>

#include<memory>

namespace vistle {
namespace insitu {
namespace sensei {

class V_SENSEIEXPORT Callbacks {
public:
	Callbacks(Mesh(*getMesh)(const std::string&), Array(*getVar)(const std::string&));
	Mesh getMesh(const std::string& name);
	Array getVar(const std::string& name);
private:
	Mesh(*m_getMesh)(const std::string&) = nullptr;
	Array(*m_getVar)(const std::string&) = nullptr;
};

class V_SENSEIEXPORT SenseiInterface {
public:

	virtual bool Execute(size_t timestep) = 0;
	virtual bool Finalize() = 0;

	virtual bool processTimestep(size_t timestep) = 0; //true if we have sth to do for the given timestep
};

std::unique_ptr<SenseiInterface> V_SENSEIEXPORT createSenseiInterface(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs, ModuleInfo moduleInfo);
}//sensei
}//insitu
}//vistle





#endif