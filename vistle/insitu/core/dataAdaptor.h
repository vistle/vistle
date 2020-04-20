#ifndef VISTLE_SENSEI_DATAADAPTOR_H
#define VISTLE_SENSEI_DATAADAPTOR_H

#include <string>
#include <vector>
#include <map>
#include <core/object.h>
#include <core/database.h>
#include <core/unstr.h>
namespace insitu {
	enum class Dimension {
		d2D = 2
		,d3D = 3
	};
	enum class CoordinateType
	{
		vertex
		,element
	};
	enum class DataType
	{
		INVALID
		,CHAR
		,INT
		,LONG
		,FLOAT
		,DOUBLE
	};
	enum class Owner {
		 Unknown
		,Simulation
		,Vistle
	};
struct Array {
	std::string name;
	size_t size;
	vistle::DataBase::Mapping centering = vistle::DataBase::Mapping::Unspecified;
	Owner owner = Owner::Unknown;
	DataType dataType = DataType::INVALID;
	void* data;
};

struct Mesh {
	bool globalView = false;
	bool interleaved = false; // if true x,y and z coordinates are interleaved in data[0]
	std::string name;
	vistle::Object::Type type = vistle::Object::Type::UNKNOWN;


	vistle::DataBase::Mapping coordinateType = vistle::DataBase::Mapping::Unspecified;
	size_t numPoints = 0; // total number of points in all blocks
	size_t numCells = 0; // total number of cells in all blocks 
	size_t cellArraySize = 0; // total cell array size in all blocks
	size_t numArrays = 0;
	std::array<size_t, 3> min, max; //outside these indicees is ghost data
	std::array<Array, 3> data;

};

struct UnstructuredMesh : public Mesh
{
	Array cl;
	Array tl;
	Array el; //must be in and will be converted to vistle::UnstructuredGrid::Type via typeToVistle
	vistle::UnstructuredGrid::Type(*typeToVistle)(int) = nullptr;

};

struct MultiMesh
{
	std::string name;
	vistle::Object::Type type = vistle::Object::Type::UNKNOWN;
	Dimension dim = Dimension::d3D;
	const int* blockIndices = nullptr; //global block indicees for the local blocks
	size_t numBlocks = 0;
	size_t numBlocksLocal = 0; // number of blocks on each rank

	size_t numGhostCells = 0; // number of ghost cell layers
	size_t numGhostNodes = 0; // number of ghost node layers
	bool periodicBoundary = false;
	bool staticMesh = false; //the mesches dont change over the timesteps, so they can be reused
	bool combine = false; //the meshes shall be combined to a single unstructured grid
	std::vector<Mesh> meshes;
};
struct DataAdaptor {
	std::vector<Mesh> meshes;
	std::map<std::string, std::vector<Array>> fieldData; //mapped to the according mesh

	size_t timestep = 0;
	float simulationTime = 0;


};


}


#endif