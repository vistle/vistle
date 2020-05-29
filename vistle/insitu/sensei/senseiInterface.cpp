#include "senseiInterface.h"
#include "senseiImpl.h"

using namespace vistle::insitu;
using namespace vistle::insitu::sensei;

std::unique_ptr<SenseiInterface> vistle::insitu::sensei::createSenseiInterface(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs, ModuleInfo moduleInfo)
{
	return std::make_unique<SenseiAdapter>(paused, rank, mpiSize, std::move(meta), cbs, moduleInfo);
}
