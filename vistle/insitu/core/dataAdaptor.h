#ifndef VISTLE_SENSEI_DATAADAPTOR_H
#define VISTLE_SENSEI_DATAADAPTOR_H

#include <string>
#include <vector>
#include <map>
#include <core/object.h>
#include <core/database.h>
#include <core/unstr.h>
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
	struct MetaData {
		std::vector<std::string> meshes;
		std::vector<std::pair<std::string, size_t>> variables; //variable and the index to the mesh it belongs to

	};

	struct Array {
		std::string name;
		size_t size;
		vistle::DataBase::Mapping centering = vistle::DataBase::Mapping::Unspecified;
		Owner owner = Owner::Unknown;
		DataType dataType = DataType::INVALID;
		void* data;
		~Array() {
			if (owner == Owner::Vistle)
			{
				free(data);
			}
		}
	};

	struct Mesh {
		Dimension dim = Dimension::d3D;
		bool globalView = false;
		bool interleaved = false; // if true x,y and z coordinates are interleaved in data[0]
		std::string name;
		vistle::Object::Type type = vistle::Object::Type::UNKNOWN;
		size_t numVerts() const {
			if (interleaved)
			{
				size_t s = 1;
				for (size_t i = 0; i < 3; i++)
				{
					s *= std::max(sizes[0], (size_t)1);
				}
				assert(data[0].size * static_cast<int>(dim) == s);
				return s;
			}
			else
			{
				assert(data[0].size == data[1].size && data[0].size == data[2].size);
				return data[0].size;
			}
		}

		vistle::DataBase::Mapping coordinateType = vistle::DataBase::Mapping::Unspecified;
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

	struct UnstructuredMesh : public Mesh
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

	struct MultiMesh : public Mesh
	{

		vistle::Object::Type type = vistle::Object::Type::PLACEHOLDER;

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

} //insitu
} //vistle


#endif