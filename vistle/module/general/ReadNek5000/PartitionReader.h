
#ifndef NEK_READEROBJECKT
#define NEK_READEROBJECKT



#include "OpenFile.h"
#include "ReaderBase.h"

namespace nek5000 {

class PartitionReader : public ReaderBase{
public:
    PartitionReader(ReaderBase &base);

    bool fillConnectivityList(vistle::Index *connectivities);
    bool fillMesh(float* x, float* y, float* z);
    bool fillVelocity(int timestep, float* x, float* y, float* z);
    bool fillScalarData(std::string varName, int timestep, float* data);
    int numberOfEqalBlockCorners();
//getter
    size_t getBlocksToRead() const;
    size_t getFirstBlockToRead() const;
    size_t getHexes()const;
    size_t getNumConn() const;
    size_t getGridSize() const;




//setter
    bool setPartition(int partition, bool useMap = true); //also starts mapping the block ids
private:
    //variables

    int myPartition;
    int myBlocksToRead; //number of blocks to read for this partition
    int myFirstBlockToRead; //first block to read

    //contains the corner points of each block
    std::map<int, std::vector < std::array<float, 3>>> blockCorners;
    // This info is for managing which blocks are read on which processors
    // and caching blocks that have been read.
    std::vector<int> myBlockIDs;
    std::vector<int> myBlockPositions;



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


    //methods
    void BlocksToRead(int totalNumBlocks, int numPartitions);

    // Cached data describing how to read data out of the file.
    std::unique_ptr<OpenFile> curOpenMeshFile, curOpenVarFile;

    //      Gets the mesh associated with this file.  The mesh is returned as a
    //      derived type of vtkDataSet (ie vtkRectilinearGrid, vtkStructuredGrid,
    //      vtkUnstructuredGrid, etc).
    bool ReadMesh(int timestep, int block, float* x, float* y, float* z);
    //read velocity in x, y and z
    bool ReadVelocity(int timestep, int block, float* x, float* y, float* z);
    //read var with varname in data
    bool ReadVar(const std::string &varname, int timestep, int block, float* data);

    //parses data files with mesh and stores the information about the block positions. If it finds a .map file use its info and returns true
    bool ReadBlockLocations(bool useMap);


    //      If the data is ascii format, there is a certain amount of deprecated
    //      data to skip at the beginning of each file.  This method tells where to
    //      seek to, to read data for the first block.
    void FindAsciiDataStart(FILE* fd, int& outDataStart, int& outLineLen);
    void getBlocksToRead(int partition);
    struct DomainParams {
        int domSizeInFloats = 0;
        int varOffsetBinary = 0;
        int varOffsetAscii = 0;
        bool timestepHasMesh = 0;
    };
    DomainParams GetDomainSizeAndVarOffset(int iTimestep, const std::string& varname);

    int getFileID(int block);
    bool CheckOpenFile(std::unique_ptr<OpenFile>& file, int timestep, int fileID);

};

}

#endif //NEK_READEROBJECKT
