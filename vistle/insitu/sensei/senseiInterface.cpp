#include "senseiInterface.h"
#include "senseiImpl.h"
#include "gridInterface.h"

#include <insitu/core/array.h>
#include <insitu/core/metaData.h>

using namespace vistle::insitu;
using namespace vistle::insitu::sensei;


sensei::Callbacks::Callbacks(std::function<Grid(const std::string&)> getGrid, std::function<Array(const std::string&)> getVar)
	:m_getGrid(getGrid)
	,m_getVar(getVar)
{
}

Grid sensei::Callbacks::getGrid(const std::string& name)
{
	return m_getGrid(name);
}

Array sensei::Callbacks::getVar(const std::string& name)
{
	return m_getVar(name);
}

std::unique_ptr<SenseiInterface> vistle::insitu::sensei::createSenseiInterface(bool paused, size_t rank, size_t mpiSize, MetaData&& meta, Callbacks cbs)
{
	return std::make_unique<SenseiAdapter>(paused, rank, mpiSize, std::move(meta), cbs);
}

vistle::insitu::sensei::SenseiInterface::~SenseiInterface()
{

}
