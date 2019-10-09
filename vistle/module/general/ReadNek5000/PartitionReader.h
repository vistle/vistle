
#ifndef NEK_READEROBJECKT
#define NEK_READEROBJECKT



#include "OpenFile.h"
#include "ReaderBase.h"

#include<set>

namespace nek5000 {

typedef std::array<int, 2> Edge;
typedef std::array<int, 4> Plane;

class PartitionReader : public ReaderBase{

public:

    PartitionReader(const ReaderBase &base);

    bool fillMesh(float* x, float* y, float* z);
    bool fillVelocity(int timestep, float* x, float* y, float* z);
    bool fillScalarData(std::string varName, int timestep, float* data);
    bool fillBlockNumbers(vistle::Index* data);
//getter
    size_t getBlocksToRead() const;
    size_t getHexes()const;
    size_t getNumConn() const;
    size_t getGridSize() const;
    size_t getNumGhostHexes() const;
    void getConnectivityList(vistle::Index* connectivityList);



//setter
    bool setPartition(int partition, int numGhostLayers, bool useMap = true); //also starts mapping the block ids
private:

    //variables
    
    int myPartition;
    std::vector<int> myBlocksToRead; //number of blocks to read for this partition
    size_t numGhostBlocks = 0;
    // This info is for managing which blocks are read on which processors
    // and caching blocks that have been read.
    std::vector<int> myBlockIDs;
    std::vector<int> myBlockPositions;
    std::vector<vistle::Index>connectivityList;
    std::array<std::vector<float>, 3> myGrid;
    int gridSize = 0;
    std::map<int, std::vector<int>> blockIndexToConnectivityIndex;
    std::set<Edge> allEdgesInCornerIndices;
    std::set<Plane> allPlanesInCornerIndices;
    //only used in parallel binary
    std::vector<int> vBlocksPerFile;


    //maps the local block-id to the rank <local blockID, fileID, position in file>
    std::map<int, std::pair<int, int>> blockMap;

    // This info is for managing which blocks are read on which processors
    // and caching blocks that have been read.
    //    std::vector<int> myBlockIDs;
    //    std::map<PointerKey, float*, KeyCompare> cachedData;
    //    std::map<int, avtIntervalTree*> boundingBoxes;
    //    std::map<PointerKey, avtIntervalTree*, KeyCompare>dataExtents;

    // Cached data describing how to read data out of the file.
    std::unique_ptr<OpenFile> curOpenMeshFile, curOpenVarFile;

    //methods

    //      Gets the mesh associated with this file.  The mesh is returned as a
    //      derived type of vtkDataSet (ie vtkRectilinearGrid, vtkStructuredGrid,
    //      vtkUnstructuredGrid, etc).
    bool ReadMesh(int timestep, int block, float* x, float* y, float* z);
    //read velocity in x, y and z
    bool ReadVelocity(int timestep, int block, float* x, float* y, float* z);
    //read var with varname in data
    bool ReadVar(const std::string &varname, int timestep, int block, float* data);

    std::vector<std::vector<float>>ReadBlock(int block);
    //parses data files with mesh and stores the information about the block positions. If it finds a .map file use its info and returns true
    bool ReadBlockLocations(bool useMap);


    //      If the data is ascii format, there is a certain amount of deprecated
    //      data to skip at the beginning of each file.  This method tells where to
    //      seek to, to read data for the first block.
    void FindAsciiDataStart(FILE* fd, int& outDataStart, int& outLineLen);

    struct DomainParams {
        int domSizeInFloats = 0;
        int varOffsetBinary = 0;
        int varOffsetAscii = 0;
        bool timestepHasMesh = 0;
    };
    DomainParams GetDomainSizeAndVarOffset(int iTimestep, const std::string& varname);

    int getFileID(int block);
    bool CheckOpenFile(std::unique_ptr<OpenFile>& file, int timestep, int fileID);
    
    void addGhostBlocks(std::vector<int> &blocksNotToRead, int numGhostLayers);
    
    bool makeConnectivityList();
    //checks if the current point is equal to the already cached point 
    bool checkPointInGridGrid(const std::array<std::vector<float>, 3> & currGrid, int currGridIndex, int gridIndex);

    std::vector<int> getMatchingPlanePoints(int block, const Plane &plane, const Plane & globalWrittenPlane);
    std::map<int, int> findAlreadyWrittenPoints(const size_t& currBlock, 
        const std::map<int, int>& writtenCorners, 
        const std::map<Edge, std::pair<bool, std::vector<int>>> writtenEdges,
        const std::map<Plane, std::pair< Plane, std::vector<int>>>& writtenPlanes,
        const std::array<std::vector<float>, 3> &currGrid);

    //returns the local block indices that belong to the corner/edge/plane given by the corner indices (c1 - c4 from 1 - 8). c1 < c2 <c3 <c4 must be true;
    std::vector<int> getIndicesBetweenCorners(std::vector<int> corners, const std::array<bool, 3> &invertAxis = { false, false, false });
    //returns the local index of a node in a block depending on its xyz block position (no coordinates!)
    int getLocalBlockIndex(int x, int y, int z);
    //return the local block index to corner index (1 - 8);
    int cornerIndexToBlockIndex(int cornerIndex); 
    //return all edges in corner indices
    void setAllEdgesInCornerIndices();
    //return all planes in corner indices
    void setAllPlanesInCornerIndices();
    //fills the connectivity list for a block, converting the already written points with localToGlobal
    void fillConnectivityList(const std::map<int, int>& localToGloabl, int startIndexInMesh);
    void constructBlockIndexToConnectivityIndex();
    //add the connectivityList entries of corners of a block(0 - myBlocksToRead.size()) to the global cornerID (from map file)
    void addNewCorners(std::map<int, int>& allCorners, int localBlock);
    void addNewEdges(std::map<Edge, std::pair<bool, std::vector<int>>>& allEdges, int localBlock);
    void addNewPlanes(std::map<Plane, std::pair< Plane, std::vector<int>>>& allEdges, int localBlock);

};

}

#endif //NEK_READEROBJECKT
