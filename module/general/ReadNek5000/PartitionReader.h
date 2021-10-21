
#ifndef NEK_READEROBJECKT
#define NEK_READEROBJECKT


#include "OpenFile.h"
#include "ReaderBase.h"

#include <set>

namespace nek5000 {

class PartitionReader: public ReaderBase {
public:
    PartitionReader(const ReaderBase &base);
    //construct the connectivity list out of the .map file(if available).
    //Reads the mesh during construction to check for "mistakes" (e.g. logical connected blocks) in the .map file
    //this updates the dimensions of the grid
    bool constructUnstructuredGrid(int timestep);
    //copies the mesh points read by constructUnstructuredMesh in the given array
    void fillGrid(std::array<vistle::Scalar *, 3> mesh) const;
    bool fillVelocity(int timestep, std::array<vistle::Scalar *, 3> data);
    bool fillScalarData(std::string varName, int timestep, vistle::Scalar *data);
    void fillBlockNumbers(vistle::Index *data) const;
    void fillConnectivityList(vistle::Index *connectivityList);
    //getter
    size_t getBlocksToRead() const;
    size_t getHexes() const;
    size_t getNumConn() const;
    size_t getGridSize() const;
    size_t getNumGhostHexes() const;
    size_t partition() const;
    //setter
    bool setPartition(int partition, unsigned numGhostLayers); //also starts mapping the block ids
private:
    //variables

    int m_partition;
    std::vector<int> m_blocksToRead; //number of blocks to read for this partition
    size_t m_numGhostBlocks = 0;

    std::vector<vistle::Index> m_connectivityList;
    std::array<std::vector<float>, 3> m_grid;
    int m_gridSize = 0;

    //only used in parallel binary
    std::vector<int> m_blocksPerFile;


    //maps the local block-id to the rank <local blockID, fileID, position in file>
    std::map<int, std::pair<int, int>> m_blockMap;

    // currently open file descriptors
    std::unique_ptr<OpenFile> m_curOpenGridFile, m_curOpenVarFile;

    //methods

    //read the mesh for the given timestep and block in x, y and z
    bool ReadGrid(int timestep, int block, float *x, float *y, float *z);
    //read velocity in x, y and z
    bool ReadVelocity(int timestep, int block, float *x, float *y, float *z);
    //read var with varname in data
    bool ReadVar(const std::string &varname, int timestep, int block, float *data);
    //parses data files with mesh and stores the information about the block positions. If it finds a .map file use its info and returns true
    bool ReadBlockLocations();


    //      If the data is ascii format, there is a certain amount of deprecated
    //      data to skip at the beginning of each file.  This method tells where to
    //      seek to, to read data for the first block.
    void FindAsciiDataStart(FILE *fd, int &outDataStart, int &outLineLen);

    struct DomainParams {
        int domSizeInFloats = 0;
        int varOffsetBinary = 0;
        int varOffsetAscii = 0;
        bool timestepHasGrid = 0;
    };
    DomainParams GetDomainSizeAndVarOffset(int iTimestep, const std::string &varname);
    //return the file index for parallel files.
    int getFileID(int block);

    bool CheckOpenFile(std::unique_ptr<OpenFile> &file, int timestep, int fileID);
    //adds numGhostLayers of block layers atound this partition and adds them to myBlocksToRead.
    void addGhostBlocks(std::vector<int> &blocksNotToRead, unsigned numGhostLayers);
    //checks if the current point is equal to the already cached point
    bool checkPointInGridGrid(const std::array<std::vector<float>, 3> &currGrid, int currGridIndex, int gridIndex);
    //returns a vector with the local block indices of the given plane in a order that matches the globalWrittenPlane's order
    std::vector<int> getMatchingPlanePoints(int block, const Plane &plane, const Plane &globalWrittenPlane);
    //maps the local block indices that also belong to already handled blocks to their corresponding position in the mesh array
    std::map<int, int>
    findAlreadyWrittenPoints(const size_t &currBlock, const std::map<int, int> &writtenCorners,
                             const std::map<Edge, std::pair<bool, std::vector<int>>> writtenEdges,
                             const std::map<Plane, std::pair<Plane, std::vector<int>>> &writtenPlanes,
                             const std::array<std::vector<float>, 3> &currGrid);

    //returns the local block indices that belong to the corner/edge/plane given by the corner indices (c1 - c4 from 1 - 8). c1 < c2 <c3 <c4 must be true;
    std::vector<int> getIndicesBetweenCorners(std::vector<int> corners,
                                              const std::array<bool, 3> &invertAxis = {false, false, false});
    //returns the local index of a node in a block depending on its xyz block position (no coordinates!)
    int getLocalBlockIndex(int x, int y, int z);
    //return the local block index to corner index (1 - 8);
    int cornerIndexToBlockIndex(int cornerIndex);

    //fills the connectivity list for a block, converting the already written points with localToGlobal
    void fillConnectivityList(const std::map<int, int> &localToGloabl, int startIndexInMesh);

    //add the connectivityList entries of corners of a block(0 - myBlocksToRead.size()) to the global cornerID (from map file)
    void addNewCorners(std::map<int, int> &allCorners, int localBlock);
    void addNewEdges(std::map<Edge, std::pair<bool, std::vector<int>>> &allEdges, int localBlock);
    void addNewPlanes(std::map<Plane, std::pair<Plane, std::vector<int>>> &allEdges, int localBlock);
};

} // namespace nek5000

#endif //NEK_READEROBJECKT
