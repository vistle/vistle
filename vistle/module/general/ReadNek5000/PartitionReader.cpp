#include "PartitionReader.h"

#include <set>
#include <iostream>
#include <fstream>
#include <cstring>
#include <array>
#include <algorithm>

using namespace std;

namespace nek5000 {
//public methods
PartitionReader::PartitionReader(const ReaderBase &base): ReaderBase((base))
{}

void PartitionReader::fillGrid(std::array<vistle::Scalar *, 3> mesh) const
{
    for (size_t i = 0; i < mesh.size(); i++) {
        if (sizeof(vistle::Scalar) == sizeof(double)) {
            std::transform(m_grid[i].begin(), m_grid[i].end(), mesh[i], [](float f) { return static_cast<double>(f); });
        } else {
            memcpy(mesh[i], m_grid[i].data(), sizeof(float) * m_gridSize);
        }
    }
}

bool PartitionReader::fillVelocity(int timestep, std::array<vistle::Scalar *, 3> data)
{
    if (m_hasVelocity) {
        int numCon = m_numCorners * m_hexesPerBlock;
        for (size_t b = 0; b < m_blocksToRead.size(); ++b) {
            array<vector<float>, 3> velocity{vector<float>(m_blockSize), vector<float>(m_blockSize),
                                             vector<float>(m_blockSize)};
            if (!ReadVelocity(timestep, m_blocksToRead[b], velocity[0].data(), velocity[1].data(),
                              velocity[2].data())) {
                return false;
            }
            for (int i = 0; i < numCon; ++i) {
                for (size_t dim = 0; dim < data.size(); dim++) {
                    data[dim][m_connectivityList[i + b * numCon]] = velocity[dim][m_connectivityList[i]];
                }
            }
        }
    }
    return true;
}

bool PartitionReader::fillScalarData(std::string varName, int timestep, vistle::Scalar *data)
{
    int numCon = m_numCorners * m_hexesPerBlock;
    for (size_t b = 0; b < m_blocksToRead.size(); b++) {
        vector<float> scalar(m_blockSize);
        if (!ReadVar(varName, timestep, m_blocksToRead[b], scalar.data())) {
            return false;
        }
        for (int i = 0; i < numCon; i++) {
            data[m_connectivityList[i + b * numCon]] = scalar[m_connectivityList[i]];
        }
    }
    return true;
}

void PartitionReader::fillBlockNumbers(vistle::Index *data) const
{
    int numConn = m_numCorners * m_hexesPerBlock;
    for (size_t i = 0; i < m_blocksToRead.size(); i++) {
        for (int j = 0; j < numConn; j++) {
            data[m_connectivityList[i * numConn + j]] = m_blocksToRead[i];
        }
    }
}

void PartitionReader::fillConnectivityList(vistle::Index *connList)
{
    memcpy(connList, m_connectivityList.data(), m_connectivityList.size() * sizeof(vistle::Index));
}

//getter
size_t PartitionReader::getBlocksToRead() const
{
    return m_blocksToRead.size();
}

size_t PartitionReader::getHexes() const
{
    return m_hexesPerBlock * m_blocksToRead.size();
}

size_t PartitionReader::getNumConn() const
{
    return m_numCorners * getHexes();
}

size_t PartitionReader::getGridSize() const
{
    return m_gridSize;
}

size_t PartitionReader::getNumGhostHexes() const
{
    return m_numGhostBlocks * m_hexesPerBlock;
}

size_t PartitionReader::partition() const
{
    return m_partition;
}


//setter
bool PartitionReader::setPartition(int partition, unsigned numGhostLayers)
{
    if (partition > m_numPartitions)
        return false;
    m_partition = partition;
    vector<int> blocksNotToRead;

    if (m_hasMap) {
        int max = m_mapFileHeader[3];
        if (m_numBlocksToRead != unsigned(m_totalNumBlocks)) {
            max = max_element(m_mapFileData.begin(), m_mapFileData.end(),
                              [](const array<int, 9> &a, const array<int, 9> &b) { return a[0] < b[0]; }) -
                  m_mapFileData.begin();
        }
        int lower = max / m_numPartitions * partition;
        int upper = lower + m_mapFileHeader[3] / m_numPartitions;
        if (partition == m_numPartitions - 1) {
            upper = max;
        }


        for (unsigned i = 0; i < m_numBlocksToRead; i++) {
            if (m_mapFileData[i][0] >= lower && m_mapFileData[i][0] < upper) {
                m_blocksToRead.push_back(i);
            } else {
                blocksNotToRead.push_back(i);
            }
        }
        addGhostBlocks(blocksNotToRead, numGhostLayers);
    } else {
        int numBlocksToRead = m_totalNumBlocks / m_numPartitions;
        int firstBlockToRead = numBlocksToRead * partition;
        int lastBlockToRead = partition == m_numPartitions - 1 ? m_totalNumBlocks : firstBlockToRead + numBlocksToRead;

        m_blocksToRead.reserve(lastBlockToRead - firstBlockToRead);
        for (int i = firstBlockToRead; i < lastBlockToRead; i++) {
            m_blocksToRead.push_back(i);
        }
        blocksNotToRead.reserve(m_totalNumBlocks - m_blocksToRead.size());
        for (int i = 0; i < m_totalNumBlocks; i++) {
            if (i < firstBlockToRead || i >= lastBlockToRead)
                blocksNotToRead.push_back(i);
        }
    }


    if (!ReadBlockLocations())
        return false;
    return true;
}

//private methods

bool PartitionReader::ReadGrid(int timestep, int block, float *x, float *y, float *z)
{
    int fileID = getFileID(block);
    if (!CheckOpenFile(m_curOpenGridFile, timestep, fileID))
        return false;

    if (m_isParallelFormat)
        block = m_blockMap[block].second;
    //block = myBlockPositions[block];

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, string());
    int nFloatsInDomain = dp.domSizeInFloats;
    long iRealHeaderSize = m_headerSize + (m_isParallelFormat ? m_blocksPerFile[fileID] * sizeof(int) : 0);

    if (m_isBinary) {
        //In the parallel format, the whole grid comes before all the vars.
        if (m_isParallelFormat)
            nFloatsInDomain = m_dim * m_blockSize;

        if (m_precision == 4) {
            fseek(m_curOpenGridFile->file(), iRealHeaderSize + (long)nFloatsInDomain * sizeof(float) * block, SEEK_SET);
            size_t res = fread(x, sizeof(float), m_blockSize, m_curOpenGridFile->file());
            (void)res;
            res = fread(y, sizeof(float), m_blockSize, m_curOpenGridFile->file());
            (void)res;
            if (m_dim == 3) {
                size_t res = fread(z, sizeof(float), m_blockSize, m_curOpenGridFile->file());
                (void)res;
            } else {
                memset(z, 0, m_blockSize * sizeof(float));
            }
            if (m_swapEndian) {
                ByteSwapArray(x, m_blockSize);
                ByteSwapArray(y, m_blockSize);
                if (m_dim == 3) {
                    ByteSwapArray(z, m_blockSize);
                }
            }

        } else {
            double *tmppts = new double[m_blockSize * m_dim];
            fseek(m_curOpenGridFile->file(), iRealHeaderSize + (long)nFloatsInDomain * sizeof(double) * block,
                  SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), m_blockSize * m_dim, m_curOpenGridFile->file());
            (void)res;
            if (m_swapEndian)
                ByteSwapArray(tmppts, m_blockSize * m_dim);
            for (unsigned i = 0; i < m_blockSize; i++) {
                x[i] = static_cast<int>(tmppts[i]);
            }
            for (unsigned i = 0; i < m_blockSize; i++) {
                y[i] = static_cast<int>(tmppts[i + m_blockSize]);
            }
            if (m_dim == 3) {
                for (unsigned i = 0; i < m_blockSize; i++) {
                    z[i] = static_cast<int>(tmppts[i + 2 * m_blockSize]);
                }
            }
            delete[] tmppts;
        }
    } else {
        for (unsigned ii = 0; ii < m_blockSize; ii++) {
            fseek(m_curOpenGridFile->file(),
                  (long)m_curOpenGridFile->iAsciiFileStart +
                      (long)block * m_curOpenGridFile->iAsciiFileLineLen * m_blockSize +
                      (long)ii * m_curOpenGridFile->iAsciiFileLineLen,
                  SEEK_SET);
            if (m_dim == 3) {
                int res = fscanf(m_curOpenGridFile->file(), " %f %f %f", &x[ii], &y[ii], &z[ii]);
                (void)res;
            } else {
                int res = fscanf(m_curOpenGridFile->file(), " %f %f", &x[ii], &y[ii]);
                (void)res;
                memset(z, 0, m_blockSize * sizeof(float));
            }
        }
    }
    //store corners
    vector<array<float, 3>> corners;

    return true;
}

bool PartitionReader::ReadVelocity(int timestep, int block, float *x, float *y, float *z)
{
    int fileID = getFileID(block);
    if (!CheckOpenFile(m_curOpenVarFile, timestep, fileID))
        return false;

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, "velocity");
    if (m_isParallelFormat)
        block = m_blockMap[block].second;
    //block = myBlockPositions[block];

    long iRealHeaderSize = m_headerSize + (m_isParallelFormat ? m_blocksPerFile[fileID] * sizeof(int) : 0);

    if (m_isBinary) {
        long filepos;
        if (!m_isParallelFormat)
            filepos = (long)iRealHeaderSize + (long)(dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else
            //This assumes [block 0: 216u 216v 216w][block 1: 216u 216v 216w]...[block n: 216u 216v 216w]
            filepos =
                (long)iRealHeaderSize +
                (long)m_blocksPerFile[fileID] * dp.varOffsetBinary * m_precision + //the header and grid if one exists
                (long)block * m_blockSize * m_dim * m_precision;
        if (m_precision == 4) {
            fseek(m_curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(x, sizeof(float), m_blockSize, m_curOpenVarFile->file());
            (void)res;
            res = fread(y, sizeof(float), m_blockSize, m_curOpenVarFile->file());
            (void)res;
            if (m_dim == 3) {
                res = fread(z, sizeof(float), m_blockSize, m_curOpenVarFile->file());
                (void)res;
            } else {
                memset(z, 0, m_blockSize * sizeof(float));
            }
            //for (size_t i = 0; i < blockSize; i++) {
            //    float v = sqrt()
            //}
            if (m_swapEndian) {
                ByteSwapArray(x, m_blockSize);
                ByteSwapArray(y, m_blockSize);
                if (m_dim == 3) {
                    ByteSwapArray(z, m_blockSize);
                }
            }
        } else {
            double *tmppts = new double[m_blockSize * m_dim];
            fseek(m_curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), m_blockSize * m_dim, m_curOpenVarFile->file());
            (void)res;

            if (m_swapEndian)
                ByteSwapArray(tmppts, m_blockSize * m_dim);

            for (unsigned ii = 0; ii < m_blockSize; ii++) {
                x[ii] = (double)tmppts[ii];
                y[ii] = (double)tmppts[ii + m_blockSize];
                if (m_dim == 3) {
                    z[ii] = (double)tmppts[ii + m_blockSize + m_blockSize];
                } else {
                    z[ii] = 0.0;
                }
            }
            delete[] tmppts;
        }
    } else {
        for (unsigned ii = 0; ii < m_blockSize; ii++) {
            fseek(m_curOpenVarFile->file(),
                  (long)m_curOpenVarFile->iAsciiFileStart +
                      (long)block * m_curOpenVarFile->iAsciiFileLineLen * m_blockSize +
                      (long)ii * m_curOpenVarFile->iAsciiFileLineLen + (long)dp.varOffsetAscii,
                  SEEK_SET);
            if (m_dim == 3) {
                int res = fscanf(m_curOpenVarFile->file(), " %f %f %f", x + ii, y + ii, z + ii);
                (void)res;
            } else {
                int res = fscanf(m_curOpenVarFile->file(), " %f %f", &x[ii], &y[ii]);
                (void)res;
                z[ii] = 0.0f;
            }
        }
    }
    return true;
}

bool PartitionReader::ReadVar(const string &varname, int timestep, int block, float *data)
{
    int fileID = getFileID(block);
    if (!CheckOpenFile(m_curOpenVarFile, timestep, fileID))
        return false;

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, varname);

    if (m_isParallelFormat)
        block = m_blockMap[block].second;
    //block = myBlockPositions[block];

    long iRealHeaderSize = m_headerSize + (m_isParallelFormat ? m_blocksPerFile[fileID] * sizeof(int) : 0);

    if (m_isBinary) {
        long filepos;
        if (!m_isParallelFormat)
            filepos = (long)iRealHeaderSize + ((long)dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else {
            // This assumes uvw for all fields comes after the grid as [block0: 216u 216v 216w]...
            // then p or t as   [block0: 216p][block1: 216p][block2: 216p]...
            if (strcmp(varname.c_str() + 2, "velocity") == 0) {
                filepos =
                    (long)iRealHeaderSize + //header
                    (long)dp.timestepHasGrid * m_blocksPerFile[fileID] * m_blockSize * m_dim * m_precision + //grid
                    (long)block * m_blockSize * m_dim * m_precision + //start of block
                    (long)(varname[0] - 'x') * m_blockSize * m_precision; //position within block
            } else
                filepos = (long)iRealHeaderSize +
                          (long)m_blocksPerFile[fileID] * dp.varOffsetBinary *
                              m_precision + //the header, grid, vel if present,
                          (long)block * m_blockSize * m_precision;
        }
        if (m_precision == 4) {
            fseek(m_curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(data, sizeof(float), m_blockSize, m_curOpenVarFile->file());
            (void)res;
            if (m_swapEndian)
                ByteSwapArray(data, m_blockSize);
        } else {
            double *tmp = new double[m_blockSize];

            fseek(m_curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(tmp, sizeof(double), m_blockSize, m_curOpenVarFile->file());
            (void)res;
            if (m_swapEndian)
                ByteSwapArray(tmp, m_blockSize);

            for (unsigned ii = 0; ii < m_blockSize; ii++)
                data[ii] = (float)tmp[ii];

            delete[] tmp;
        }
    } else {
        float *var_tmp = data;
        for (unsigned ii = 0; ii < m_blockSize; ii++) {
            fseek(m_curOpenVarFile->file(),
                  (long)m_curOpenVarFile->iAsciiFileStart +
                      (long)block * m_curOpenVarFile->iAsciiFileLineLen * m_blockSize +
                      (long)ii * m_curOpenVarFile->iAsciiFileLineLen + (long)dp.varOffsetAscii,
                  SEEK_SET);
            int res = fscanf(m_curOpenVarFile->file(), " %f", var_tmp);
            (void)res;
            var_tmp++;
        }
    }
    return true;
}

bool PartitionReader::ReadBlockLocations()
{
    // In each parallel file, in the header, there's a table that maps
    // each local block to a global id which starts at 1.  Here, I make
    // an inverse map, from a zero-based global id to a proc num and local
    // offset.

    if (!m_isBinary || !m_isParallelFormat)
        return true;

    m_blocksPerFile = std::vector<int>(m_numOutputDirs);
    std::fill(m_blocksPerFile.begin(), m_blocksPerFile.end(), 0);


    string blockfilename;
    std::vector<int> tmpBlocks(m_totalNumBlocks);
    ifstream f;
    int badFile = m_totalNumBlocks + 1;
    for (int ii = 0; ii < m_numOutputDirs; ++ii) {
        blockfilename = GetFileName(0, ii);
        f.open(blockfilename, ifstream::binary);
        if (!f.is_open()) {
            badFile = ii;
            break;
        }

        int tmp1, tmp2, tmp3, tmp4;
        f.seekg(5, std::ios_base::beg); //seek past the #std
        f >> tmp1 >> tmp2 >> tmp3 >> tmp4 >> m_blocksPerFile[ii];

        f.seekg(136, std::ios_base::beg);
        f.read((char *)tmpBlocks.data() + ii * m_blocksPerFile[ii], m_blocksPerFile[ii] * sizeof(int));
        if (m_swapEndian)
            ByteSwapArray(tmpBlocks.data() + ii * m_blocksPerFile[ii], m_blocksPerFile[ii]);


        f.close();

        for (int jj = ii * m_blocksPerFile[ii]; jj < (ii + 1) * m_blocksPerFile[ii]; ++jj) {
            int iBlockID = tmpBlocks[jj] - 1;
            if (iBlockID < 0 || iBlockID >= m_totalNumBlocks) {
                sendError(" .nek500: Error reading parallel file block IDs.");
                return false;
            }
            m_blockMap[iBlockID] = std::make_pair(ii, jj);
        }
    }

    //badFile = UnifyMinimumValue(badFile);
    if (badFile < m_totalNumBlocks) {
        blockfilename = GetFileName(0, badFile);
        sendError("Could not open file " + blockfilename + " to read block locations.");
        return false;
    }

    //Do a sanity check
    int sum = 0;

    for (int ii = 0; ii < m_numOutputDirs; ii++)
        sum += m_blocksPerFile[ii];

    if (sum != m_totalNumBlocks) {
        sendError("nek5000: Sum of blocks per file does not equal total number of blocks");
        return false;
    }
    return true;
}

void PartitionReader::FindAsciiDataStart(FILE *fd, int &outDataStart, int &outLineLen)
{
    //Skip the header, then read a float for each block.  Then skip beyond the
    //newline character and return the current position.
    fseek(fd, m_headerSize, SEEK_SET);
    for (int ii = 0; ii < m_totalNumBlocks; ii++) {
        float dummy;
        int res = fscanf(fd, " %f", &dummy);
        (void)res;
    }
    char tmp[1024];
    char *res = nullptr;
    (void)res;
    res = fgets(tmp, 1023, fd);
    outDataStart = ftell(fd);

    res = fgets(tmp, 1023, fd);
    outLineLen = ftell(fd) - outDataStart;
}

PartitionReader::DomainParams PartitionReader::GetDomainSizeAndVarOffset(int iTimestep, const string &var)
{
    DomainParams params;
    params.timestepHasGrid = 0;

    if (m_timestepsWithGrid[iTimestep] == true)
        params.timestepHasGrid = 1;

    int nFloatsPerSample = 0;
    if (params.timestepHasGrid)
        nFloatsPerSample += m_dim;
    if (m_hasVelocity)
        nFloatsPerSample += m_dim;
    if (m_hasPressure)
        nFloatsPerSample += 1;
    if (m_hasTemperature)
        nFloatsPerSample += 1;
    nFloatsPerSample += m_numScalarFields;

    params.domSizeInFloats = nFloatsPerSample * m_blockSize;

    if (var.length() > 0) {
        int iNumPrecedingFloats = 0;
        if (var == "velocity" || var == "velocity_mag" || var == "x_velocity") {
            if (params.timestepHasGrid)
                iNumPrecedingFloats += m_dim;
        } else if (var == "y_velocity") {
            if (params.timestepHasGrid)
                iNumPrecedingFloats += m_dim;

            iNumPrecedingFloats += 1;
        } else if (var == "z_velocity") {
            if (params.timestepHasGrid)
                iNumPrecedingFloats += m_dim;

            iNumPrecedingFloats += 2;
        } else if (var == "pressure") {
            if (params.timestepHasGrid)
                iNumPrecedingFloats += m_dim;
            if (m_hasVelocity)
                iNumPrecedingFloats += m_dim;
        } else if (var == "temperature") {
            if (params.timestepHasGrid)
                iNumPrecedingFloats += m_dim;
            if (m_hasVelocity)
                iNumPrecedingFloats += m_dim;
            if (m_hasPressure)
                iNumPrecedingFloats += 1;
        } else if (var[0] == 's') {
            if (params.timestepHasGrid)
                iNumPrecedingFloats += m_dim;
            if (m_hasVelocity)
                iNumPrecedingFloats += m_dim;
            if (m_hasPressure)
                iNumPrecedingFloats += 1;
            if (m_hasTemperature)
                iNumPrecedingFloats += 1;

            int iSField = atoi(var.c_str() + 1);
            //iSField should be between 1..iNumSFields, inclusive
            iNumPrecedingFloats += iSField - 1;
        }
        params.varOffsetBinary = m_blockSize * iNumPrecedingFloats;
        params.varOffsetAscii = 14 * iNumPrecedingFloats;
    }
    return params;
}

int PartitionReader::getFileID(int block)
{
    int fileID = 0;
    if (m_isParallelFormat)
        fileID = m_blockMap[block].first;
    return fileID;
}

bool PartitionReader::CheckOpenFile(std::unique_ptr<OpenFile> &file, int timestep, int fileID)
{
    if (timestep < 0) {
        return false;
    }
    string filename = GetFileName(timestep, fileID);

    if (!file || file->name() != filename) {
        file.reset(new OpenFile(filename));
        if (!file->file())
            return false;
        if (!m_isBinary)
            FindAsciiDataStart(file->file(), file->iAsciiFileStart, file->iAsciiFileLineLen);
    }
    return true;
}

void PartitionReader::addGhostBlocks(std::vector<int> &blocksNotToRead, unsigned numGhostLayers)
{
    if (m_blocksToRead.size() == m_numBlocksToRead) {
        return;
    }
    m_numGhostBlocks = 0;
    for (size_t iterations = 0; iterations < numGhostLayers; iterations++) {
        int oldNumBlocks = m_blocksToRead.size();
        for (int myBlock = 0; myBlock < oldNumBlocks; ++myBlock) {
            for (auto i = m_mapFileData[m_blocksToRead[myBlock]].begin() + 1;
                 i < m_mapFileData[m_blocksToRead[myBlock]].end(); i++) {
                {
                    auto notMyBlock = blocksNotToRead.begin();
                    while (notMyBlock != blocksNotToRead.end()) {
                        if (find(m_mapFileData[*notMyBlock].begin() + 1, m_mapFileData[*notMyBlock].end(), *i) !=
                            m_mapFileData[*notMyBlock].end()) {
                            m_blocksToRead.push_back(*notMyBlock);
                            notMyBlock = blocksNotToRead.erase(notMyBlock);
                        } else {
                            ++notMyBlock;
                        }
                    }
                }
            }
        }
        m_numGhostBlocks += m_blocksToRead.size() - oldNumBlocks;
    }
}

bool PartitionReader::constructUnstructuredGrid(int timestep)
{
    /*
    local block indices
    __________
   7         8
  /         /|
 /         / |
/_________/  |
5         6  |
|    _____|__|
|   3     |  4
|  /      | /
| /       |/
1_________2
*/
    //maps from  (sorted) global (from map file) cornerid(s) to cornerindex(indices) in connectivity list
    map<int, int> writtenCorners;
    map<Edge, pair<bool, vector<int>>>
        writtenEdges; //key is sorted global indices, bool is wheter it had to be sorted, vector contains the indices to the coordinates in mesh file
    map<Plane, pair<Plane, vector<int>>>
        writtenPlanes; //key is sorted global indices, pair contains unsorted globl indices and a vector containing the indices to the coordinates in mesh file

    m_connectivityList.clear();
    m_connectivityList.reserve(getNumConn());
    m_gridSize = m_blockSize * m_blocksToRead.size();
    int numDoublePoints = 0;


    for (size_t i = 0; i < 3; i++) {
        m_grid[i].resize(m_blocksToRead.size() * m_blockSize);
    }
    for (size_t currBlock = 0; currBlock < m_blocksToRead.size(); currBlock++) {
        //pre-read the grid to verify overlapping corners. The is necessary because the mapfile also contains not physically (logically) linked blocks
        array<vector<float>, 3> grid{vector<float>(m_blockSize), vector<float>(m_blockSize),
                                     vector<float>(m_blockSize)};
        if (!ReadGrid(timestep, m_blocksToRead[currBlock], grid[0].data(), grid[1].data(), grid[2].data())) {
            return false;
        }
        //contains the points, that are already written(in local block indices) and where to find them in the coordinate list and a set of new edges that have to be reversed
        map<int, int> localToGloabl =
            m_hasMap ? findAlreadyWrittenPoints(currBlock, writtenCorners, writtenEdges, writtenPlanes, grid)
                     : map<int, int>{};
        fillConnectivityList(localToGloabl, currBlock * m_blockSize - numDoublePoints);
        numDoublePoints += localToGloabl.size();
        if (m_hasMap) {
            //add my corners/edges/planes to the "already written" maps
            addNewCorners(writtenCorners, currBlock);
            addNewEdges(writtenEdges, currBlock);
            if (m_dim == 3) {
                addNewPlanes(writtenPlanes, currBlock);
            }
        }

        //add the new connectivityList entries to my grid
        int numCon = m_numCorners * m_hexesPerBlock;
        for (int i = 0; i < numCon; i++) {
            for (int j = 0; j < 3; j++) {
                m_grid[j][m_connectivityList[i + currBlock * numCon]] = grid[j][m_connectivityList[i]];
            }
        }
    }
    m_gridSize -= numDoublePoints;
    return true;
}

bool PartitionReader::checkPointInGridGrid(const array<vector<float>, 3> &currGrid, int currGridIndex, int gridIndex)
{
    for (size_t i = 0; i < 3; i++) {
        float diff = currGrid[i][currGridIndex] - m_grid[i][gridIndex];
        if (diff * diff > 0.00001) {
            return false;
        }
    }
    return true;
}

std::vector<int> PartitionReader::getMatchingPlanePoints(int block, const Plane &plane, const Plane &globalWrittenPlane)
{
    //find edges that form a plane
    block = m_blocksToRead[block];
    Plane writtenPlane;
    for (size_t i = 0; i < 4; i++) {
        writtenPlane[i] = find(m_mapFileData[block].begin() + 1, m_mapFileData[block].end(), globalWrittenPlane[i]) -
                          m_mapFileData[block].begin();
    }
    vector<vector<int>> corners{vector<int>{plane[0], plane[1]}, vector<int>{plane[2], plane[3]}};
    vector<vector<int>> writtenCorners{vector<int>{writtenPlane[0], writtenPlane[1]},
                                       vector<int>{writtenPlane[2], writtenPlane[3]}};
    std::array<bool, 3> invertedAxes{false, false, false};
    if (corners != writtenCorners) {
        if (plane[3] - plane[0] == 3) { //xy plane
            if (writtenCorners[0] == corners[1]) { //y inverted
                invertedAxes[0] = true;
            } else { //x inverted
                invertedAxes[1] = true;
            }
        }
        if (plane[3] - plane[0] == 5) { //xz plane
            if (writtenCorners[0] == corners[1]) { //x inverted
                invertedAxes[2] = true;
            } else { //z inverted
                invertedAxes[0] = true;
            }
        }
        if (plane[3] - plane[0] == 6) { //yz plane
            if (writtenCorners[0] == corners[1]) { //y inverted
                invertedAxes[2] = true;
            } else { //z inverted
                invertedAxes[1] = true;
            }
        }
    }
    vector<int> p(4);
    copy(plane.begin(), plane.end(), p.begin());

    return getIndicesBetweenCorners(p, invertedAxes);
}

std::map<int, int> PartitionReader::findAlreadyWrittenPoints(const size_t &currBlock,
                                                             const std::map<int, int> &writtenCorners,
                                                             const map<Edge, pair<bool, vector<int>>> writtenEdges,
                                                             const map<Plane, pair<Plane, vector<int>>> &writtenPlanes,
                                                             const std::array<std::vector<float>, 3> &currGrid)
{
    std::map<int, int> localToGloabl;
    map<Edge, vector<int>> myWrittenEdges; //the already written edges of this block and their indices in the grid
    auto corners = m_mapFileData[m_blocksToRead[currBlock]]; //corner info starts at index 1
    //Lookup if a corner of this block has already been written
    for (size_t i = 1; i < corners.size(); i++) {
        auto ci = writtenCorners.find(corners[i]);
        int blockIndex = cornerIndexToBlockIndex(i);
        if (ci != writtenCorners.end() &&
            checkPointInGridGrid(currGrid, blockIndex, ci->second)) { //corner already written
            localToGloabl[blockIndex] = ci->second;
        }
    }
    if (localToGloabl.size() > 1) {
        //check for edges
        for (Edge edge: m_allEdgesInCornerIndices) {
            Edge globalEdge;
            for (size_t i = 0; i < 2; i++) {
                globalEdge[i] = corners[edge[i]];
            }
            auto unsortedEdge = globalEdge;
            sort(globalEdge.begin(), globalEdge.end());
            bool sorted = unsortedEdge == globalEdge ? false : true;


            auto it = writtenEdges.find(globalEdge);
            if (it != writtenEdges.end()) {
                auto e = myWrittenEdges.insert({edge, it->second.second}).first;
                if (it->second.first != sorted) { //keep the points sorted
                    reverse(e->second.begin(), e->second.end());
                }
            }
        }
    }

    //check and insert planes before eges are inserted because edges may be in a reversed order
    if (m_dim == 3 && myWrittenEdges.size() > 3) {
        //check for planes
        for (Plane plane: m_allPlanesInCornerIndices) {
            Plane globalPlane;
            vector<int> localPlane(4);
            for (size_t i = 0; i < 4; i++) {
                globalPlane[i] = corners[plane[i]];
                localPlane[i] = plane[i];
            }
            sort(globalPlane.begin(), globalPlane.end());
            auto it = writtenPlanes.find(globalPlane);
            if (it != writtenPlanes.end()) {
                auto globalPoints = it->second.second;
                auto localPoints = getMatchingPlanePoints(currBlock, plane, it->second.first);
                for (size_t i = 0; i < globalPoints.size(); i++) {
                    if (checkPointInGridGrid(currGrid, localPoints[i], globalPoints[i])) {
                        localToGloabl[localPoints[i]] = globalPoints[i];
                    }
                }
            }
        }
    }
    for (auto edge: myWrittenEdges) {
        auto localPoints = getIndicesBetweenCorners({edge.first[0], edge.first[1]});
        for (size_t i = 0; i < edge.second.size(); i++) {
            if (checkPointInGridGrid(currGrid, localPoints[i], edge.second[i])) {
                localToGloabl[localPoints[i]] = edge.second[i];
            }
        }
    }

    return localToGloabl;
}

vector<int> PartitionReader::getIndicesBetweenCorners(std::vector<int> corners, const std::array<bool, 3> &invertAxis)
{
    if (corners.size() == 1) {
        return vector<int>{cornerIndexToBlockIndex(corners[0])};
    }
    if (corners.size() == 2) {
        vector<int> indices;
        int myDim = m_blockDimensions[0] - 1;
        if (corners[1] - corners[0] == 2) {
            myDim = m_blockDimensions[1] - 1;
        } else if (corners[1] - corners[0] == 4) {
            myDim = m_blockDimensions[2] - 1;
        }

        int ci1 = cornerIndexToBlockIndex(corners[0]);
        int ci2 = cornerIndexToBlockIndex(corners[1]);
        int step = (ci2 - ci1) / myDim;
        for (int i = ci1; i <= ci2; i += step) {
            indices.push_back(i);
        }

        return indices;
    }
    if (corners.size() == 4) {
        vector<int> indices;
        if (corners[3] - corners[0] == 3) { //xy plane
            vector<int> in;
            for (int i = cornerIndexToBlockIndex(corners[0]); i <= cornerIndexToBlockIndex(corners[3]); i++) {
                indices.push_back(i);
            }
            if (invertAxis[0]) {
                for (unsigned i = 0; i < m_blockDimensions[1]; i++) {
                    vector<int> inv;
                    inv.reserve(m_blockDimensions[0]);
                    for (int j = m_blockDimensions[0] - 1; j >= 0; --j) {
                        inv.push_back(indices[i * m_blockDimensions[0] + j]);
                    }
                    copy(inv.begin(), inv.end(), indices.begin() + i * m_blockDimensions[0]);
                }
            }
            if (invertAxis[1]) {
                int dim1 = m_blockDimensions[0];
                int dim2 = m_blockDimensions[1];
                for (int i = 0; i < dim2 / 2; i++) {
                    vector<int> tmp(dim1);
                    copy(indices.begin() + i * dim1, indices.begin() + (i + 1) * dim1, tmp.begin());
                    copy(indices.end() - (i + 1) * dim1, indices.end() - i * dim1, indices.begin() + i * dim1);
                    copy(tmp.begin(), tmp.end(), indices.end() - (i + 1) * dim1);
                }
            }
        } else if (corners[3] - corners[0] == 5) { //xz plane
            int start = cornerIndexToBlockIndex(corners[0]);
            for (unsigned i = 0; i < m_blockDimensions[2]; i++) {
                for (unsigned j = 0; j < m_blockDimensions[1]; j++) {
                    indices.push_back(start + j);
                }
                start += m_blockDimensions[0] * m_blockDimensions[1];
            }
            if (invertAxis[0]) {
                for (unsigned i = 0; i < m_blockDimensions[2]; i++) {
                    vector<int> inv;
                    inv.reserve(m_blockDimensions[0]);
                    for (int j = m_blockDimensions[0] - 1; j >= 0; --j) {
                        inv.push_back(indices[i * m_blockDimensions[0] + j]);
                    }
                    copy(inv.begin(), inv.end(), indices.begin() + i * m_blockDimensions[0]);
                }
            }
            if (invertAxis[2]) {
                int dim1 = m_blockDimensions[0];
                int dim2 = m_blockDimensions[2];
                for (int i = 0; i < dim2 / 2; i++) {
                    vector<int> tmp(dim1);
                    copy(indices.begin() + i * dim1, indices.begin() + (i + 1) * dim1, tmp.begin());
                    copy(indices.end() - (i + 1) * dim1, indices.end() - i * dim1, indices.begin() + i * dim1);
                    copy(tmp.begin(), tmp.end(), indices.end() - (i + 1) * dim1);
                }
            }
        } else if (corners[3] - corners[0] == 6) { //yz plane
            for (int i = cornerIndexToBlockIndex(corners[0]); i <= cornerIndexToBlockIndex(corners[3]);
                 i += m_blockDimensions[0]) {
                indices.push_back(i);
            }
            if (invertAxis[1]) {
                for (unsigned i = 0; i < m_blockDimensions[2]; i++) {
                    vector<int> inv;
                    inv.reserve(m_blockDimensions[1]);
                    for (int j = m_blockDimensions[1] - 1; j >= 0; j--) {
                        inv.push_back(indices[i * m_blockDimensions[1] + j]);
                    }
                    copy(inv.begin(), inv.end(), indices.begin() + i * m_blockDimensions[1]);
                }
            }
            if (invertAxis[2]) {
                int dim1 = m_blockDimensions[1];
                int dim2 = m_blockDimensions[2];
                for (int i = 0; i < dim2 / 2; i++) {
                    vector<int> tmp(dim1);
                    copy(indices.begin() + i * dim1, indices.begin() + (i + 1) * dim1, tmp.begin());
                    copy(indices.end() - (i + 1) * dim1, indices.end() - i * dim1, indices.begin() + i * dim1);
                    copy(tmp.begin(), tmp.end(), indices.end() - (i + 1) * dim1);
                }
            }
        }
        return indices;
    }
    cerr << "nek5000: getBlockIndices failed" << endl;
    return vector<int>();
}

int PartitionReader::getLocalBlockIndex(int x, int y, int z)
{
    int index = x;
    index += y * m_blockDimensions[0];
    index += z * m_blockDimensions[0] * m_blockDimensions[1];
    return index;
}

int PartitionReader::cornerIndexToBlockIndex(int cornerIndex)
{
    switch (cornerIndex) {
    case (1):
        return getLocalBlockIndex(0, 0, 0);
    case (2):
        return getLocalBlockIndex(m_blockDimensions[0] - 1, 0, 0);
    case (3):
        return getLocalBlockIndex(0, m_blockDimensions[1] - 1, 0);
    case (4):
        return getLocalBlockIndex(m_blockDimensions[0] - 1, m_blockDimensions[1] - 1, 0);
    case (5):
        return getLocalBlockIndex(0, 0, m_blockDimensions[2] - 1);
    case (6):
        return getLocalBlockIndex(m_blockDimensions[0] - 1, 0, m_blockDimensions[2] - 1);
    case (7):
        return getLocalBlockIndex(0, m_blockDimensions[1] - 1, m_blockDimensions[2] - 1);
    case (8):
        return getLocalBlockIndex(m_blockDimensions[0] - 1, m_blockDimensions[1] - 1, m_blockDimensions[2] - 1);
    default:
        break;
    }

    assert(cornerIndex >= 1 && cornerIndex <= 8);

    return getLocalBlockIndex(0, 0, 0);
}

void PartitionReader::fillConnectivityList(const map<int, int> &localToGloabl, int startIndexInGrid)
{
    vector<vistle::Index> connList(m_numCorners * m_hexesPerBlock);
    int numDoubles = 0;

    for (size_t index = 0; index < m_blockSize; index++) {
        auto conListEntries = m_blockIndexToConnectivityIndex.find(index)->second;
        auto it = localToGloabl.find(index);
        if (it != localToGloabl.end()) {
            for (auto entry: conListEntries) {
                connList[entry] = it->second;
            }
            ++numDoubles;
        } else {
            for (auto entry: conListEntries) {
                connList[entry] = startIndexInGrid + index - numDoubles;
            }
        }
    }
    m_connectivityList.insert(m_connectivityList.end(), connList.begin(), connList.end());
}

void PartitionReader::addNewCorners(map<int, int> &allCorners, int localBlock)
{
    auto corners = m_mapFileData[m_blocksToRead[localBlock]];
    for (int i = 1; i <= m_numCorners; i++) {
        allCorners[corners[i]] =
            m_connectivityList[m_blockIndexToConnectivityIndex.find(cornerIndexToBlockIndex(i))->second[0] +
                               localBlock * m_numCorners * m_hexesPerBlock];
    }
}

void PartitionReader::addNewEdges(map<Edge, pair<bool, vector<int>>> &allEdges, int localBlock)
{
    auto corners = m_mapFileData[m_blocksToRead[localBlock]];
    for (auto edge: m_allEdgesInCornerIndices) {
        Edge e = {corners[edge[0]], corners[edge[1]]};
        auto unsortedE = e;
        sort(e.begin(), e.end());
        bool s = unsortedE == e ? false : true;

        auto it = allEdges.find(e);
        if (it == allEdges.end()) {
            vector<int> blockIndices = getIndicesBetweenCorners({edge[0], edge[1]});
            vector<int> gridIndices;
            gridIndices.reserve(blockIndices.size());
            for (int blockIndex: blockIndices) {
                gridIndices.push_back(m_connectivityList[m_blockIndexToConnectivityIndex.find(blockIndex)->second[0] +
                                                         localBlock * m_numCorners * m_hexesPerBlock]);
            }
            allEdges[e] = make_pair(s, gridIndices);
        }
    }
}

void PartitionReader::addNewPlanes(map<Plane, pair<Plane, vector<int>>> &allPlanes, int localBlock)
{
    auto corners = m_mapFileData[m_blocksToRead[localBlock]];
    for (auto plane: m_allPlanesInCornerIndices) {
        vector<int> blockIndices = getIndicesBetweenCorners({plane[0], plane[1], plane[2], plane[3]});
        vector<int> gridIndices;
        gridIndices.reserve(blockIndices.size());
        for (int blockIndex: blockIndices) {
            gridIndices.push_back(m_connectivityList[m_blockIndexToConnectivityIndex.find(blockIndex)->second[0] +
                                                     localBlock * m_numCorners * m_hexesPerBlock]);
        }


        Plane e = {corners[plane[0]], corners[plane[1]], corners[plane[2]], corners[plane[3]]};
        auto unsortedE = e;
        sort(e.begin(), e.end());
        allPlanes[e] = make_pair(unsortedE, gridIndices);
    }
}

} // namespace nek5000
