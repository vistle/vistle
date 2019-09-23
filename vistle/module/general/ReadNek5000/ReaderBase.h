#ifndef NEK_READER_BASE_H
#define NEK_READER_BASE_H

#include <vector>
#include <map>
#include <memory>

#include <util/byteswap.h>
#include<core/index.h>

namespace nek5000 {
class ReaderBase
{
public:
    ReaderBase(std::string file, int numPartitions, int blocksToRead);
    virtual ~ReaderBase();
    bool init();
//getter
    size_t getNumTimesteps() const;
    size_t getNumScalarFields() const;
    int getDim() const;
    bool hasVelocity() const;
    bool hasPressure() const;
    bool hasTemperature() const;

protected:
    //variables
    std::string file; //path to meta data file
    std::string fileTemplate; //template for datafiles

    int numPartitions = 0;
    int numBlocksToRead = 0; //blocks to read given by user input
    int firstTimestep = 1;
    int numTimesteps = 1;
    int blockDimensions[3];
    int blockSize = 0;
    int dim = 2;
    int totalNumBlocks = 0; //total number of blocks per timestep
    int iNumberOfRanks = 1;

    bool isBinary = false;         //binary or ascii
    int numOutputDirs = 1;  //number of parallel files per timestep
    bool isParalellFormat = false;

    int numberOfTimePeriods = 1;
    double gapBetweenTimePeriods = 0.0;

    // This info is embedded in, or derived from, the file header
    bool bSwapEndian = false;

    int iNumBlocks = 0;
    vistle::Index iBlockSize[3]{ 1,1,1 };
    bool bHasVelocity = false;
    bool bHasPressure = false;
    bool bHasTemperature = false;
    int numScalarFields = 0;
    int iHeaderSize = 0;
    int iDim = 3;
    int iPrecision = 4; //4 or 8 for float or double
    // This info is distributed through all the dumps, and only
    // computed on demand
    std::vector<int> aCycles;
    std::vector<double> aTimes;
    std::vector<bool> readTimeInfoFor;
    std::vector<bool> vTimestepsWithMesh;
    int curTimestep = 1;
    int timestepToUseForMesh = 0;

    std::vector<int>all_element_list;
    //methods

    //      This method is called as part of initialization.  It parses the text
    //      file which is a companion to the series of .fld files that make up a
    //      dataset.
    bool parseMetaDataFile();
    //      This method is called as part of initialization.  Some of the file
    //      metadata is written in the header of each timestep, and this method
    //      reads and parses that metadata.
    bool ParseNekFileHeader();
    //if there is a .map file, use it to partition the grid
    bool ParseGridMap();

    //      Parses the characters in a binary Nek header file that indicate which
    //  fields are present.  Tags can be separated by 0 or more spaces.  Parsing
    //  stops when an unknown character is encountered, or when the file pointer
    //  moves beyond this->iHeaderSize characters.
    //
    //  X or X Y Z indicate a mesh.  Ignored here, UpdateCyclesAndTimes picks this up.
    //  U indicates velocity
    //  P indicates pressure
    //  T indicates temperature
    //  1 2 3 or S03  indicate 3 other scalar fields
    void ParseFieldTags(std::ifstream& f);

    void UpdateCyclesAndTimes();

    //      Create a filename from the template and other info.
    std::string GetFileName(int rawTimestep, int pardir);

    template<class T>
    void ByteSwapArray(T* val, int size)
    {
        for (int i = 0; i < size; i++)
        {
            val[i] = vistle::byte_swap<vistle::endianness::little_endian, vistle::endianness::big_endian>(val[i]);
        }
    }
        void sendError(const std::string &msg);
};
}//nek5000

#endif //NEK_READER_BASE_H
