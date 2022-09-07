#ifndef NEK_READER_BASE_H
#define NEK_READER_BASE_H

#include <vector>
#include <map>
#include <memory>
#include <array>
#include <set>

#include <vistle/util/byteswap.h>
#include <vistle/core/index.h>
#include <vistle/core/scalar.h>

namespace nek5000 {
typedef std::array<int, 2> Edge;
typedef std::array<int, 4> Plane;

//this class reads and stores general data that is independent of timesteps and partitions
class ReaderBase {
public:
    ReaderBase(std::string file, int numPartitions, int blocksToRead);
    virtual ~ReaderBase();
    //reads metadata file, a datafile header and mapfile. Returns false on failure
    bool init();
    //getter
    size_t getNumTimesteps() const;
    size_t getNumScalarFields() const;
    int getDim() const;
    bool hasVelocity() const;
    bool hasPressure() const;
    bool hasTemperature() const;
    void UpdateCyclesAndTimes(int timestep);
    bool hasGrid(int timestep) const;

protected:
    //variables
    std::string file; //path to meta data file
    std::string fileTemplate; //template for datafiles

    int m_numPartitions = 0;
    unsigned m_numBlocksToRead = 0; //blocks to read given by user input
    int m_firstTimestep = 1;
    int m_numTimesteps = 1;
    unsigned m_blockDimensions[3] = {1, 1, 1};
    unsigned m_blockSize = 0;
    int m_dim = 1;
    int m_numCorners = 0;
    int m_hexesPerBlock = 0;
    int m_totalNumBlocks = 0; //total number of blocks per timestep
    int m_numberOfRanks = 1;
    bool m_isBinary = false; //binary or ascii
    int m_numOutputDirs = 1; //number of parallel files per timestep
    bool m_isParallelFormat = false;

    int m_numberOfTimePeriods = 1;
    double m_gapBetweenTimePeriods = 0.0;

    //connectivity list of a single isolated block
    std::vector<vistle::Index> m_baseConnList;
    std::map<int, std::vector<int>> m_blockIndexToConnectivityIndex;
    std::set<Edge> m_allEdgesInCornerIndices;
    std::set<Plane> m_allPlanesInCornerIndices;


    // This info is embedded in, or derived from, the file header
    bool m_swapEndian = false;

    bool m_hasVelocity = false;
    bool m_hasPressure = false;
    bool m_hasTemperature = false;
    int m_numScalarFields = 0;
    int m_headerSize = 0;
    int m_precision = 4; //4 or 8 for float or double
    // This info is distributed through all the dumps, and only
    // computed on demand
    std::vector<int> m_cycles;
    std::vector<double> m_times;
    std::vector<bool> m_readTimeInfoFor;
    std::vector<bool> m_timestepsWithGrid;
    int m_defaultTimestepToReadGrid = 0;
    //header of map file containing: numBlocks, numUniqeEdges, depth, maxNumPartitions(2^dept), ?, ?
    bool m_hasMap = false;
    std::array<int, 7> m_mapFileHeader;
    //contains the data from the .map file
    std::vector<std::array<int, 9>> m_mapFileData; //toDo: sort out blocks that do not belong to this partition
    //methods

    template<class T>
    void ByteSwapArray(T *val, int size)
    {
        for (int i = 0; i < size; i++) {
            val[i] = vistle::byte_swap<vistle::endianness::little_endian, vistle::endianness::big_endian>(val[i]);
        }
    }
    //Create a filename from the template and other info.
    std::string GetFileName(int rawTimestep, int pardir);
    void sendError(const std::string &msg);


private:
    //methods
    //creates a list of all possible corners in edge indices
    void setAllEdgesInCornerIndices();
    //creates a list of all possible planes in edge indices
    void setAllPlanesInCornerIndices();
    //creates a map that contains all
    void setBlockIndexToConnectivityIndex();


    //      This method is called as part of initialization.  It parses the text
    //      file which is a companion to the series of .fld files that make up a
    //      dataset.
    bool parseMetaDataFile();
    //      This method is called as part of initialization.  Some of the file
    //      metadata is written in the header of each timestep, and this method
    //      reads and parses that metadata.
    bool ParseNekFileHeader();
    //if there is a .map file, use it to partition the grid and to connect blocks
    bool ParseGridMap();

    //      Parses the characters in a binary Nek header file that indicate which
    //  fields are present.  Tags can be separated by 0 or more spaces.  Parsing
    //  stops when an unknown character is encountered, or when the file pointer
    //  moves beyond this->iHeaderSize characters.
    //
    //  X or X Y Z indicate a grid.  Ignored here, UpdateCyclesAndTimes picks this up.
    //  U indicates velocity
    //  P indicates pressure
    //  T indicates temperature
    //  1 2 3 or S03  indicate 3 other scalar fields
    void ParseFieldTags(std::ifstream &f);

    void makeBaseConnList();
};
} // namespace nek5000

#endif //NEK_READER_BASE_H
