#ifndef VISTLE_SENSEI_STRUCTURED_GRID_BASE
#define VISTLE_SENSEI_STRUCTURED_GRID_BASE

#include "export.h"

#include <insitu/core/array.h>

#include <array>

namespace vistle {
namespace insitu {
namespace sensei {
class V_SENSEIEXPORT StructuredGridBase {
public:
	StructuredGridBase(std::array<vistle::insitu::Array, 3>&& data, const std::array<size_t, 3>& dimensions); //create Mesh with separate data for each dimension
	StructuredGridBase(std::array<vistle::insitu::Array, 3>&& data); //set the dimensions based on the array sizes
	StructuredGridBase(vistle::insitu::Array&& data, const std::array<size_t, 3> dimensions); //create Mesh from interleaved data

	void setGhostCells(const std::array<size_t, 3> &min, const std::array<size_t, 3> &max);

	void setGlobalBlockIndex(size_t index);


protected:
	bool m_interleaved = false;
	bool m_hasGhost = false;
	size_t m_globalBlockIndex = 0;
	std::array<size_t, 3> m_sizes{ 0,0,0 }; //these are the dimensions
	mutable std::array<size_t, 3> m_min{}, m_max{};  //out of these bounds is ghost data
	std::array<vistle::insitu::Array, 3> m_data; //if interleaved only data[0] is used

	unsigned int dim() const;
	void resetGhostToNoGhost() const;
	bool checkGhost() const; //check if ghost data is valid

};

}
}
}



#endif // !VISTLE_SENSEI_STRUCTURED_GRID_BASE
