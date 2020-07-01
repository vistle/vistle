#ifndef VISTLE_INSITU_MESH_H
#define VISTLE_INSITU_MESH_H
#include "exeption.h"
#include "array.h"

#include <core/index.h>
#include <core/scalar.h>

#include <array>
#include <vector>


namespace vistle {

namespace insitu {
namespace message {
class SyncShmIDs;
}
enum class Dimension { //castable to int
	d2D = 2
	, d3D = 3
};



enum class MeshType {
	Invalid
	, Unstructured
	, Uniform
	, Rectilinear
	, Structured
	, Multi
};





class V_INSITUCOREEXPORT MeshObject {
public:
	MeshObject();
	MeshObject(const std::string& name, MeshType type, std::array<Array, 3>&& data = std::array<Array, 3>{}); //create Mesh with separate data for each dimension
	MeshObject(const std::string& name, MeshType type, Array&& data, std::array<size_t, 3> dimensions); //create Mesh from interleaved data

	//Mesh(const Mesh& other) = delete;
	//Mesh(Mesh&& other) noexcept = default;
	//Mesh& operator=(const Mesh& other) = delete;
	//Mesh& operator=(Mesh&& other) noexcept = default;

	virtual ~MeshObject() = default;


	void setData(std::array<Array, 3>&& data); //set arrays with separate data for each dimension
	void setData(Array&& data, std::array<size_t, 3> dimensions); //set arrays with interleaved data
	void setGostCells(std::array<size_t, 3> min, std::array<size_t, 3> max);
	size_t numVerts() const;

	Dimension dim() const;
	bool isInterleaved() const;// if true x,y and z coordinates are interleaved in data[0]
	std::string name() const;
	MeshType type() const;
	DataMapping mapping() const;
	size_t size(unsigned int index) const;
	const Array& data(unsigned int index) const;
	const std::array<size_t, 3>& lowerGhostBound() const;//below these indicees is ghost data
	const std::array<size_t, 3>& upperGhostBound() const;//uppon these indicees is ghost data
	void setGlobalBlockIndex(size_t index);
	size_t globalBlockIndex() const;

private:
	bool m_interleaved = false; 
	bool m_hasGhost = false;
	std::string m_name = "blabla";
	MeshType m_type = MeshType::Invalid;
	size_t m_globalBlockIndex = 0;
	std::array<size_t, 3> m_sizes{ 0,0,0 }; //these are the dimensions
	mutable std::array<size_t, 3> m_min, m_max;  //out of these bounds is ghost data
	std::array<Array, 3> m_data; //if interleaved only data[0] is used
	
	
	void initGhostToNoGhost() const;
	bool checkGhost() const; //check if ghost data is valid

};


typedef  std::unique_ptr<MeshObject> Mesh;
class V_INSITUCOREEXPORT MultiMeshObject : public MeshObject
{
public:
	typedef std::vector<Mesh>::const_iterator Iter;
	MultiMeshObject(const std::string& name);
	Iter addMesh(Mesh&& mesh, size_t globalBlockIndex);
	Iter begin() const;
	Iter end() const;
	size_t size() const;
private:


	std::vector<size_t> m_globalBlockIndices; //global block indicees for the local blocks
	size_t m_numBlocksGlobal = 0;
	size_t m_numBlocksLocal = 0; // number of blocks on each rank

	size_t m_numGhostCells = 0; // number of ghost cell layers
	size_t m_numGhostNodes = 0; // number of ghost node layers
	bool m_periodicBoundary = false;
	bool m_staticMesh = false; //the mesches dont change over the timesteps, so they can be reused
	bool m_combine = false; //the meshes shall be combined to a single unstructured grid

	std::vector<Mesh> m_meshes;
};
typedef  std::unique_ptr<MultiMeshObject> MultiMesh;
}
}

#endif // !VISTLE_INSITU_MESH_H
