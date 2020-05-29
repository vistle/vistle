#ifndef VISTLE_SENSEI_DATAADAPTOR_H
#define VISTLE_SENSEI_DATAADAPTOR_H

#include "export.h"

#include <string>
#include <vector>
#include <map>
#include <array>


#include <core/index.h>
#include <core/scalar.h>


namespace vistle {
namespace insitu {
	enum class Dimension { //castable to int
		d2D = 2
		, d3D = 3
	};
	enum class CoordinateType
	{
		vertex
		, element
	};
	enum class DataType
	{
		INVALID
		, CHAR
		, INT
		, LONG
		, FLOAT
		, DOUBLE
	};
	enum class Owner {
		Unknown
		, Simulation
		, Vistle
	};
	enum class DataMapping {
		Unspecified
		, Element
		,Vertex

	};

	enum class MeshType {
		Unknown
		,Unstructured
		,Uniform
		,Rectilinear
		,Structured
		,Multi
	};


	struct V_INSITUCOREEXPORT MetaData {
		std::vector<std::string> meshes;
		std::vector<std::pair<std::string, size_t>> variables; //variable and the index to the mesh it belongs to

	};

	struct V_INSITUCOREEXPORT Array {
		std::string name;
		size_t size;
		DataMapping mapping = DataMapping::Unspecified;
		Owner owner = Owner::Unknown;
		DataType dataType = DataType::INVALID;
		void* data;
		~Array();
	};

	struct V_INSITUCOREEXPORT Mesh {
		Dimension dim = Dimension::d3D;
		bool globalView = false;
		bool interleaved = false; // if true x,y and z coordinates are interleaved in data[0]
		std::string name;
		MeshType type = MeshType::Unknown;
		size_t numVerts() const;

		DataMapping mapping = DataMapping::Unspecified;
		size_t blockNumber = 0;
		size_t numPoints = 0; // total number of points in all blocks
		size_t numCells = 0; // total number of cells in all blocks 
		size_t cellArraySize = 0; // total cell array size in all blocks
		size_t numArrays = 0;
		std::array<size_t, 3> sizes; //if iterleaved these are the dimensions
		std::array<size_t, 3> min, max; //outside these indicees is ghost data
		std::array<Array, 3> data; //if interleaved only data[0] is used
		virtual ~Mesh() {};

	};

	struct V_INSITUCOREEXPORT UnstructuredMesh : public Mesh
	{
		//types are void and will be converted with ...ToVistle function;
		Array cl;
		Array tl;
		Array el;
		//functions to convert cl, tl and el to vistle
		bool (*clToVistle)(vistle::Index* vistleTl);
		bool (*tlToVistle)(vistle::Byte* vistleTl);
		bool (*elToVistle)(vistle::Index* vistleTl);

	};

	struct V_INSITUCOREEXPORT MultiMesh : public Mesh
	{

		MeshType type = MeshType::Multi;

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
	struct V_INSITUCOREEXPORT DataAdaptor {
		std::vector<Mesh> meshes;
		std::map<std::string, std::vector<Array>> fieldData; //mapped to the according mesh

		size_t timestep = 0;
		float simulationTime = 0;
	};

} //insitu
} //vistle


#endif