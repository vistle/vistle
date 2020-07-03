#ifndef VISTLE_SENSEI_GRID_INTERFACE_H
#define VISTLE_SENSEI_GRID_INTERFACE_H

#include "export.h"

#include <memory>
#include <vector>
#include <string>

namespace vistle {
class Object;
typedef std::shared_ptr<const Object> Object_const_ptr;
namespace insitu {
namespace message {
class SyncShmIDs;
}
namespace sensei {

class V_SENSEIEXPORT GridInterface {
public:
	GridInterface(const GridInterface& other) = delete;
	GridInterface(GridInterface&& other) = default;
	void operator=(const GridInterface& other) = delete;
	GridInterface& operator=(GridInterface&& other) = default;

	virtual const std::unique_ptr<GridInterface>* getGrid(size_t index) const{
		return nullptr;
	}
	virtual size_t numGrids() const {
		return 0;
	}
	virtual vistle::Object_const_ptr toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const = 0;
	const std::string& name() const{
		return m_name;
	}
protected:
	GridInterface(const std::string& name) :m_name(name) {}
private:
	const std::string m_name;
};
typedef std::unique_ptr<GridInterface> Grid;
}
}
}
#endif // !VISTLE_SENSEI_GRID_INTERFACE_H
