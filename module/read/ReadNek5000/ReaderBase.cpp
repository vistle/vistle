#include "ReaderBase.h"


#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>


#ifdef _WIN32
#include <direct.h>
#endif // _WIN32

using namespace std;
namespace nek5000 {

ReaderBase::ReaderBase(std::string file, int numPartitions, int blocksToRead)
: file(file), m_numPartitions(numPartitions), m_numBlocksToRead(blocksToRead)
{}

ReaderBase::~ReaderBase()
{}

bool ReaderBase::init()
{
    if (!parseMetaDataFile() || !ParseNekFileHeader()) {
        return false;
    }
    ParseGridMap();
    if (m_numBlocksToRead < 1 || m_numBlocksToRead > unsigned(m_totalNumBlocks))
        m_numBlocksToRead = m_totalNumBlocks;
    UpdateCyclesAndTimes(0);
    //create basic structures to later create connectivity lists
    makeBaseConnList();
    setAllPlanesInCornerIndices();
    setAllEdgesInCornerIndices();
    setBlockIndexToConnectivityIndex();

    return true;
}
//getter
size_t ReaderBase::getNumTimesteps() const
{
    return m_numTimesteps;
}

size_t ReaderBase::getNumScalarFields() const
{
    return m_numScalarFields;
}

int ReaderBase::getDim() const
{
    return m_dim;
}

bool ReaderBase::hasVelocity() const
{
    return m_hasVelocity;
}

bool ReaderBase::hasPressure() const
{
    return m_hasPressure;
}

bool ReaderBase::hasTemperature() const
{
    return m_hasTemperature;
}

//protected methods
std::string ReaderBase::GetFileName(int rawTimestep, int pardir)
{
    int timestep = rawTimestep + m_firstTimestep;
    int nPrintfTokens = 0;

    for (size_t ii = 0; ii < fileTemplate.size() - 1; ii++) {
        if (fileTemplate[ii] == '%' && fileTemplate[ii + 1] != '%')
            nPrintfTokens++;
    }

    if (nPrintfTokens > 1) {
        m_isBinary = true;
        m_isParallelFormat = true;
    }

    if (!m_isParallelFormat && nPrintfTokens != 1) {
        sendError("Nek: The filetemplate tag must receive only one printf token for serial Nek files.");
    } else if (m_isParallelFormat && (nPrintfTokens < 2 || nPrintfTokens > 3)) {
        sendError("Nek: The filetemplate tag must receive either 2 or 3 printf tokens for parallel Nek files.");
    }
    int bufSize = fileTemplate.size();
    int len;
    string s;
    do {
        bufSize += 64;
        char *outFileName = new char[bufSize];
        if (!m_isParallelFormat)
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), timestep);
        else if (nPrintfTokens == 2)
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), pardir, timestep);
        else
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), pardir, pardir, timestep);
        s = string(outFileName, len);
        delete[] outFileName;
    } while (len >= bufSize);
    return s;
}

void ReaderBase::sendError(const std::string &msg)
{
    cerr << msg << endl;
}

void ReaderBase::UpdateCyclesAndTimes(int timestep)
{
    if (m_times.size() != (size_t)m_numTimesteps) {
        m_times.resize(m_numTimesteps);
        m_cycles.resize(m_numTimesteps);
        m_timestepsWithGrid.resize(m_numTimesteps, false);
        m_timestepsWithGrid[0] = true;
        m_readTimeInfoFor.resize(m_numTimesteps, false);
    }
    ifstream f;
    char dummy[64];
    double t;
    int c;
    string v;
    t = 0.0;
    c = 0;

    string gridFilename = GetFileName(timestep, 0);
    f.open(gridFilename.c_str());

    if (!m_isParallelFormat) {
        string tString, cString;
        f >> dummy >> dummy >> dummy >> dummy >> tString >> cString >> v; //skip #blocks and block size
        t = atof(tString.c_str());
        c = atoi(cString.c_str());
    } else {
        f >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy;
        f >> t >> c >> dummy;

        //I do this to skip the num directories token, because it may abut
        //the field tags without a whitespace separator.
        while (f.peek() == ' ')
            f.get();
        while (f.peek() >= '0' && f.peek() <= '9')
            f.get();

        char tmpTags[32];
        f.read(tmpTags, 32);
        tmpTags[31] = '\0';

        v = tmpTags;
    }
    f.close();

    m_times[timestep] = t;
    m_cycles[timestep] = c;
    //if (metadata != nullptr) {
    //    metadata->SetTime(timestep + timeSliceOffset, t);
    //    metadata->SetTimeIsAccurate(true, timestep + timeSliceOffset);
    //    metadata->SetCycle(timestep + timeSliceOffset, c);
    //    metadata->SetCycleIsAccurate(true, timestep + timeSliceOffset);
    //}

    // If this file contains a grid, the first variable codes after the
    // cycle number will be X Y
    if (v.find("X") != string::npos) {
        m_timestepsWithGrid[timestep] = true;
    }

    // Nek has a bug where the time and cycle sometimes run together (e.g. 2.52000E+0110110 for
    // time 25.2, cycle 10110).  If that happens, then v will be Y
    if (v.find("Y") != string::npos)
        m_timestepsWithGrid[timestep] = true;

    m_readTimeInfoFor[timestep] = true;
}

bool ReaderBase::hasGrid(int timestep) const
{
    return m_timestepsWithGrid[timestep];
}


//private methods
void ReaderBase::setAllEdgesInCornerIndices()
{
    m_allEdgesInCornerIndices.insert({1, 2});
    m_allEdgesInCornerIndices.insert({1, 3});
    m_allEdgesInCornerIndices.insert({2, 4});
    m_allEdgesInCornerIndices.insert({3, 4});
    if (m_dim == 3) {
        m_allEdgesInCornerIndices.insert({1, 5});
        m_allEdgesInCornerIndices.insert({2, 6});
        m_allEdgesInCornerIndices.insert({3, 7});
        m_allEdgesInCornerIndices.insert({4, 8});
        m_allEdgesInCornerIndices.insert({5, 6});
        m_allEdgesInCornerIndices.insert({5, 7});
        m_allEdgesInCornerIndices.insert({6, 8});
        m_allEdgesInCornerIndices.insert({7, 8});
    }
}

void ReaderBase::setAllPlanesInCornerIndices()
{
    vector<Plane> planes;
    m_allPlanesInCornerIndices.insert({1, 2, 3, 4});
    m_allPlanesInCornerIndices.insert({1, 2, 5, 6});
    m_allPlanesInCornerIndices.insert({1, 3, 5, 7});
    m_allPlanesInCornerIndices.insert({2, 4, 6, 8});
    m_allPlanesInCornerIndices.insert({3, 4, 7, 8});
    m_allPlanesInCornerIndices.insert({5, 6, 7, 8});
}

void ReaderBase::setBlockIndexToConnectivityIndex()
{
    m_blockIndexToConnectivityIndex.clear();
    for (size_t j = 0; j < m_blockSize; j++) {
        vector<int> connectivityIndices;
        auto index = m_baseConnList.begin(), end = m_baseConnList.end();
        while (index != end) {
            index = std::find(index, end, j);
            if (index != end) {
                int i = index - m_baseConnList.begin();
                connectivityIndices.push_back(i);
                ++index;
            }
        }
        m_blockIndexToConnectivityIndex[j] = connectivityIndices;
    }
}


bool ReaderBase::parseMetaDataFile()
{
    string tag;
    char buf[2048];
    ifstream f(file);
    int ii;

    // Process a tag at a time until all lines have been read
    while (f.good()) {
        f >> tag;
        if (f.eof()) {
            f.clear();
            break;
        }

        if (tag[0] == '#') {
            f.getline(buf, 2048);
            continue;
        }

        if (tag == "endian:") {
            //This tag is deprecated.  There's a float written into each binary file
            //from which endianness can be determined.
            string dummy_endianness;
            f >> dummy_endianness;
        } else if (tag == "filetemplate:") {
            f >> fileTemplate;
        } else if (tag == "firsttimestep:") {
            f >> m_firstTimestep;
        } else if (tag == "numtimesteps:") {
            f >> m_numTimesteps;
        } else if (tag == "meshcoords:") {
            //This tag is now deprecated.  The same info can be discovered by
            //this reader while it scans all the headers for time and cycle info.

            int nStepsWithCoords;
            f >> nStepsWithCoords;

            for (ii = 0; ii < nStepsWithCoords; ii++) {
                int step;
                f >> step;
            }
        } else if (tag == "type:") {
            //This tag can be deprecated, because the type can be determined
            //from the header
            string t;
            f >> t;
            if (tag == "binary") {
                m_isBinary = true;
            } else if (tag == "binary6") {
                m_isBinary = true;
                m_isParallelFormat = true;
            } else if (tag == "ascii") {
                m_isBinary = false;
            } else {
                sendError(".nek5000: Value following \"type\" must be \"ascii\" or \"binary\" or \"binary6\"");
                return false;
            }
        } else if (tag == "numoutputdirs:") {
            //This tag can be deprecated, because the number of output dirs
            //can be determined from the header, and the parallel nature of the
            //file can be inferred from the number of printf tokens in the template.
            //This reader scans the headers for the number of fld files
            f >> m_numOutputDirs;
            if (m_numOutputDirs > 1)
                m_isParallelFormat = true;
        } else if (tag == "timeperiods:") {
            f >> m_numberOfTimePeriods;
        } else if (tag == "gapBetweenTimePeriods:") {
            f >> m_gapBetweenTimePeriods;
        } else if (tag == "NEK3D") {
            //This is an obsolete tag, ignore it.
        } else if (tag == "version:") {
            //This is an obsolete tag, ignore it, skipping the version number
            //as well.
            string version;
            f >> version;
        } else {
            sendError(".nek5000: Error parsing file.  Unknown tag " + tag);
            return false;
        }
    }

    if (m_numberOfTimePeriods < 1) {
        sendError(".nek5000: The number of time periods must be 1 or more.");
        return false;
    }
    if (m_numberOfTimePeriods > 1 && m_gapBetweenTimePeriods <= 0.0) {
        sendError(".nek5000: The gap between time periods must be non-zero.");
        return false;
    }

    //Do a little consistency checking before moving on
    if (fileTemplate == "") {
        sendError(".nek5000: A tag called filetemplate: must be specified");
        return false;
    }

    if (f.is_open())
        f.close();

    // make the file template, which is normally relative to the file being opened,
    // into an absolute path
    if (fileTemplate[0] != '/') {
        for (ii = file.size(); ii >= 0; ii--) {
            if (file[ii] == '/' || file[ii] == '\\') {
                fileTemplate.insert(0, file.c_str(), ii + 1);
                break;
            }
        }
        if (ii == -1) {
#ifdef _WIN32
            _getcwd(buf, 512);
#else
            char *res = getcwd(buf, 512);
            (void)res;
#endif
            strcat(buf, "/");
            fileTemplate.insert(0, buf, strlen(buf));
        }
    }

#ifdef _WIN32
    for (ii = 0; ii < fileTemplate.size(); ii++) {
        if (fileTemplate[ii] == '/')
            fileTemplate[ii] = '\\';
    }
#endif
    return true;
}

bool ReaderBase::ParseGridMap()
{
    string base_filename = file;
    size_t ext = base_filename.find_last_of('.') + 1;
    base_filename.erase(ext);
    string map_filename = base_filename + "map";
    ifstream mptr(map_filename);
    if (mptr.is_open()) {
        for (size_t i = 0; i < 7; i++) {
            mptr >> m_mapFileHeader[i];
        }
        m_mapFileData.reserve(m_mapFileHeader[0]);
        int columns = m_dim == 2 ? 5 : 9;
        for (int i = 0; i < m_mapFileHeader[0]; ++i) {
            array<int, 9> line;
            for (int j = 0; j < columns; j++) {
                mptr >> line[j];
            }
            m_mapFileData.emplace_back(line);
        }
        mptr.close();
        m_hasMap = true;
        return true;
    }
    string ma2_filename = base_filename + "ma2";
    ifstream ma2ptr(ma2_filename, ifstream::binary);
    if (ma2ptr.is_open()) {
        string version;
        ma2ptr >> version;
        if (version != "#v001") {
            sendError("ma2 file of version " + version + " is not supported");
            return false;
        }
        for (size_t i = 0; i < 7; i++) {
            ma2ptr >> m_mapFileHeader[i];
        }
        ma2ptr.seekg(132 / 4 * sizeof(float), ios::beg);
        char test;
        ma2ptr >> test; //to do: use this to perform byteswap if necessary
        ma2ptr.seekg((132 / 4 + 1) * sizeof(float), ios::beg);
        m_mapFileData.reserve(m_mapFileHeader[0]);
        int columns = m_dim == 2 ? 5 : 9;

        int *map = new int[columns * m_mapFileHeader[0]];
        ma2ptr.read((char *)map, columns * m_mapFileHeader[0] * sizeof(float));

        for (int i = 0; i < m_mapFileHeader[0]; ++i) {
            array<int, 9> line;
            for (int j = 0; j < columns; j++) {
                line[j] = map[i * columns + j];
            }
            m_mapFileData.emplace_back(line);
        }
        delete[] map;
        ma2ptr.close();
        m_hasMap = true;
    }


    sendError("ReadNek: can neither open map file " + map_filename + " nor ma2 file " + ma2_filename);
    return false;
}

bool ReaderBase::ParseNekFileHeader()
{
    string buf2, tag;

    //Now read the header of one of the files to get block and variable info
    string blockfilename = GetFileName(0, 0);
    ifstream f(blockfilename, ifstream::binary);

    if (!f.is_open()) {
        sendError("Could not open file " + file + ", which should exist according to header file " + blockfilename +
                  ".");
        return false;
    }

    // Determine the type (ascii or binary)
    // Parallel type is determined by the number of tokens in the file template
    // and is always binary
    if (!m_isParallelFormat) {
        float test;
        f.seekg(80, ios::beg);

        f.read((char *)(&test), 4);
        if (test > 6.5 && test < 6.6)
            m_isBinary = true;
        else {
            ByteSwapArray(&test, 1);
            if (test > 6.5 && test < 6.6)
                m_isBinary = true;
        }
        f.seekg(0, ios::beg);
    }

    //iHeaderSize no longer includes the size of the block index metadata, for the
    //parallel format, since this now can vary per file.
    if (m_isBinary && m_isParallelFormat)
        m_headerSize = 136;
    else if (m_isBinary && !m_isParallelFormat)
        m_headerSize = 84;
    else
        m_headerSize = 80;


    if (!m_isParallelFormat) {
        f >> m_totalNumBlocks;
        f >> m_blockDimensions[0];
        f >> m_blockDimensions[1];
        f >> m_blockDimensions[2];

        f >> buf2; //skip
        f >> buf2; //skip

    } else {
        //Here's are some examples of what I'm parsing:
        //#std 4  6  6  6   120  240  0.1500E+01  300  1  2XUPT
        //#std 4  6  6  6   120  240  0.1500E+01  300  1  2 U T123
        //This example means:  #std is for versioning, 4 bytes per sample,
        //  6x6x6 blocks, 120 of 240 blocks are in this file, time=1.5,
        //  cycle=300, this output dir=1, num output dirs=2, XUPT123 are
        //  tags that this file has a grid, velocity, pressure, temperature,
        //  and 3 misc scalars.
        //
        //A new revision of the binary header changes the way tags are
        //represented.  Line 2 above would be represented as
        //#std 4  6  6  6   120  240  0.1500E+01  300  1  2UTS03
        //The spaces between tags are removed, and instead of representing
        //scalars as 123, they use S03, allowing more than 9 total.
        f >> tag;
        if (tag != "#std") {
            sendError("Nek: Error reading the header.  Expected it to start with #std");
            return false;
        }
        f >> m_precision;
        f >> m_blockDimensions[0];
        f >> m_blockDimensions[1];
        f >> m_blockDimensions[2];
        f >> buf2; //blocks per file
        f >> m_totalNumBlocks;

        //This bypasses some tricky and unnecessary parsing of data
        //I already have.
        //6.13.08  No longer works...
        //f.seekg(77, std::ios_base::beg);
        f >> buf2; //time
        f >> buf2; //cycle
        f >> buf2; //directory num of this file

        //I do this to skip the num directories token, because it may abut
        //the field tags without a whitespace separator.
        while (f.peek() == ' ')
            f.get();

        //The number of output dirs comes next, but may abut the field tags
        m_numOutputDirs = 0;
        while (f.peek() >= '0' && f.peek() <= '9') {
            m_numOutputDirs *= 10;
            m_numOutputDirs += (f.get() - '0');
        }
    }
    ParseFieldTags(f);
    if (m_totalNumBlocks < 0) {
        sendError("negative total block count");
        m_totalNumBlocks = 0;
    }
    m_dim = m_blockDimensions[2] == 1 ? 2 : 3;
    m_numCorners = m_dim == 2 ? 4 : 8;
    m_blockSize = m_blockDimensions[0] * m_blockDimensions[1] * m_blockDimensions[2];
    m_hexesPerBlock = (m_blockDimensions[0] - 1) * (m_blockDimensions[1] - 1);
    if (m_dim == 3)
        m_hexesPerBlock *= (m_blockDimensions[2] - 1);
    if (m_isBinary) {
        // Determine endianness and whether we need to swap bytes.
        // If this machine's endian matches the file's, the read will
        // put 6.54321 into this float.

        float test;
        if (!m_isParallelFormat) {
            f.seekg(80, std::ios_base::beg);
            f.read((char *)(&test), 4);
        } else {
            f.seekg(132, std::ios_base::beg);
            f.read((char *)(&test), 4);
        }
        if (test > 6.5 && test < 6.6)
            m_swapEndian = false;
        else {
            ByteSwapArray(&test, 1);
            if (test > 6.5 && test < 6.6)
                m_swapEndian = true;
            else {
                sendError("Nek: Error reading file, while trying to determine endianness.");
                return false;
            }
        }
    }
    return true;
}

void ReaderBase::ParseFieldTags(ifstream &f)
{
    m_numScalarFields = 1;
    int numSpacesInARow = 0;
    bool foundCoordinates = false;
    while (f.tellg() < m_headerSize) {
        char c = f.get();
        if (numSpacesInARow >= 5)
            continue;
        if (c == ' ') {
            numSpacesInARow++;
            continue;
        }
        numSpacesInARow = 0;
        if (c == 'X' || c == 'Y' || c == 'Z') {
            foundCoordinates = true;
            continue;
        } else if (c == 'U')
            m_hasVelocity = true;
        else if (c == 'P')
            m_hasPressure = true;
        else if (c == 'T')
            m_hasTemperature = true;
        else if (c >= '1' && c <= '9') {
            // If we have S##, then it will be caught in the 'S'
            // logic below.  So this means that we have the legacy
            // representation and there is between 1 and 9 scalars.
            m_numScalarFields = c - '0';
        } else if (c == 'S') {
            while (f.peek() == ' ')
                f.get();
            char digit1 = f.get();
            while (f.peek() == ' ')
                f.get();
            char digit2 = f.get();

            if (digit1 >= '0' && digit1 <= '9' && digit2 >= '0' && digit2 <= '9')
                m_numScalarFields = (digit1 - '0') * 10 + (digit2 - '0');
            else
                m_numScalarFields = 1;
        } else
            break;
    }
    if (!foundCoordinates) {
        sendError("Nek: The first time step in a Nek file must contain a grid");
    }
}

void ReaderBase::makeBaseConnList()
{
    using vistle::Index;
    m_baseConnList.resize(m_numCorners * m_hexesPerBlock);
    Index *nl = m_baseConnList.data();
    for (Index ii = 0; ii < m_blockDimensions[0] - 1; ii++) {
        for (Index jj = 0; jj < m_blockDimensions[1] - 1; jj++) {
            if (m_dim == 2) {
                *nl++ = jj * (m_blockDimensions[0]) + ii;
                *nl++ = jj * (m_blockDimensions[0]) + ii + 1;
                *nl++ = (jj + 1) * (m_blockDimensions[0]) + ii + 1;
                *nl++ = (jj + 1) * (m_blockDimensions[0]) + ii;
            } else {
                for (Index kk = 0; kk < m_blockDimensions[2] - 1; kk++) {
                    *nl++ = kk * (m_blockDimensions[1]) * (m_blockDimensions[0]) + jj * (m_blockDimensions[0]) + ii;
                    *nl++ = kk * (m_blockDimensions[1]) * (m_blockDimensions[0]) + jj * (m_blockDimensions[0]) + ii + 1;
                    *nl++ = kk * (m_blockDimensions[1]) * (m_blockDimensions[0]) + (jj + 1) * (m_blockDimensions[0]) +
                            ii + 1;
                    *nl++ =
                        kk * (m_blockDimensions[1]) * (m_blockDimensions[0]) + (jj + 1) * (m_blockDimensions[0]) + ii;
                    *nl++ =
                        (kk + 1) * (m_blockDimensions[1]) * (m_blockDimensions[0]) + jj * (m_blockDimensions[0]) + ii;
                    *nl++ = (kk + 1) * (m_blockDimensions[1]) * (m_blockDimensions[0]) + jj * (m_blockDimensions[0]) +
                            ii + 1;
                    *nl++ = (kk + 1) * (m_blockDimensions[1]) * (m_blockDimensions[0]) +
                            (jj + 1) * (m_blockDimensions[0]) + ii + 1;
                    *nl++ = (kk + 1) * (m_blockDimensions[1]) * (m_blockDimensions[0]) +
                            (jj + 1) * (m_blockDimensions[0]) + ii;
                }
            }
        }
    }
}
} // namespace nek5000
