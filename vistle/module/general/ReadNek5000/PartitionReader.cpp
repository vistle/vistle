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
PartitionReader::PartitionReader(const ReaderBase &base)
    :ReaderBase((base))
{

}

bool PartitionReader::fillMesh(float *x, float *y, float *z)
{
    memcpy(x, myGrid[0].data(), sizeof(float) * gridSize);
    memcpy(y, myGrid[1].data(), sizeof(float) * gridSize);
    memcpy(z, myGrid[2].data(), sizeof(float) * gridSize);
    myGrid.fill(vector<float>());
    return true;
}

bool PartitionReader::fillVelocity(int timestep, float *x, float *y, float *z)
{
    if (bHasVelocity) {
        int numCon = numCorners * hexesPerBlock;
        for (size_t b = 0; b < myBlocksToRead.size(); ++b) {
            array<vector<float>, 3> velocity{ vector<float>(blockSize), vector<float>(blockSize) , vector<float>(blockSize) };
            if (!ReadVelocity(timestep, myBlocksToRead[b], velocity[0].data(), velocity[1].data(), velocity[2].data())) {
                return false;
            }
            for (size_t i = 0; i < numCon; ++i) {

                x[connectivityList[i + b * numCon]] = velocity[0][connectivityList[i]];
                y[connectivityList[i + b * numCon]] = velocity[1][connectivityList[i]];
                z[connectivityList[i + b * numCon]] = velocity[2][connectivityList[i]];
            }

        }
    }
    return true;
}

bool PartitionReader::fillScalarData(std::string varName, int timestep, float *data)
{
    int numCon = numCorners * hexesPerBlock;
    for (int b = 0; b < myBlocksToRead.size(); b++) {
        vector<float> scalar(blockSize);
        if (!ReadVar(varName, timestep, myBlocksToRead[b], scalar.data())) {
            return false;
        }
        for (size_t i = 0; i < numCon; i++) {

            data[connectivityList[i + b * numCon]] = scalar[connectivityList[i]];
        }
    }
    return true;
}

bool PartitionReader::fillBlockNumbers(vistle::Index* data) {
    int numConn = numCorners * hexesPerBlock;
    for (size_t i = 0; i < myBlocksToRead.size(); i++) {
        for (size_t j = 0; j < numConn; j++) {
            data[connectivityList[i * numConn + j]] = myBlocksToRead[i];
        }
    }
    return true;;
}

void PartitionReader::fillConnectivityList(vistle::Index* connList) {
    memcpy(connList, connectivityList.data(), connectivityList.size() * sizeof(vistle::Index));
}

//getter
size_t PartitionReader::getBlocksToRead() const
{
    return myBlocksToRead.size();
}

size_t PartitionReader::getHexes()const
{
    return hexesPerBlock * myBlocksToRead.size();
}

size_t PartitionReader::getNumConn() const
{
        return numCorners * getHexes();
}

size_t PartitionReader::getGridSize() const
{
    return gridSize;
}

size_t PartitionReader::getNumGhostHexes() const {
    return numGhostBlocks * hexesPerBlock;
}


//setter
bool PartitionReader::setPartition(int partition, int numGhostLayers, bool useMap)
{
    if(partition > numPartitions)
        return false;
    myPartition = partition;
    
    int max = mapFileHeader[3];
    if (numBlocksToRead != totalNumBlocks) {

        max = max_element(mapFileData.begin(), mapFileData.end(),
            [](const array<int, 9> & a, const array<int, 9> & b) {return a[0] < b[0]; }) - mapFileData.begin();
    }
    int lower = max / numPartitions * partition; 
    int upper = lower + mapFileHeader[3] / numPartitions;
    if (partition == numPartitions -1) {
        upper = max;
    }


    vector<int> blocksNotToRead;
    for (size_t i = 0; i < numBlocksToRead; i++) { 
        if (mapFileData[i][0] >= lower && mapFileData[i][0] < upper) {
            myBlocksToRead.push_back(i);
        }
        else {
            blocksNotToRead.push_back(i);
        }
    }
    addGhostBlocks(blocksNotToRead, numGhostLayers);
    if(!ReadBlockLocations())
        return false;
    makeConnectivityList();
    return true;
}

//private methods

bool PartitionReader::ReadMesh(int timestep, int block, float *x, float *y, float* z) {
    int fileID = getFileID(block);
    if(!CheckOpenFile(curOpenMeshFile, timestep, fileID))
        return false;

    if (isParalellFormat)
        block = blockMap[block].second;
    //block = myBlockPositions[block];

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, string());
    int nFloatsInDomain = dp.domSizeInFloats;
    long iRealHeaderSize = iHeaderSize + (isParalellFormat ? vBlocksPerFile[fileID] * sizeof(int) : 0);

    if (isBinary) {
        //In the parallel format, the whole mesh comes before all the vars.
        if (isParalellFormat)
            nFloatsInDomain = dim * blockSize;

        if (iPrecision == 4) {
             fseek(curOpenMeshFile->file(), iRealHeaderSize + (long)nFloatsInDomain * sizeof(float) * block, SEEK_SET);
            size_t res = fread(x, sizeof(float), blockSize, curOpenMeshFile->file()); (void)res;
            res = fread(y, sizeof(float), blockSize, curOpenMeshFile->file()); (void)res;
            if (dim == 3) {
                size_t res = fread(z, sizeof(float), blockSize, curOpenMeshFile->file()); (void)res;
            }
            else {
                memset(z, 0, blockSize * sizeof(float));
            }
            if (bSwapEndian)                 {
                ByteSwapArray(x, blockSize);
                ByteSwapArray(y, blockSize);
                if (dim == 3) {
                    ByteSwapArray(z, blockSize);
                }
            }

        } else {
            double* tmppts = new double[blockSize * dim];
            fseek(curOpenMeshFile->file(), iRealHeaderSize +
                (long)nFloatsInDomain * sizeof(double) * block,
                SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), blockSize * dim, curOpenMeshFile->file()); (void)res;
            if (bSwapEndian)
                ByteSwapArray(tmppts, blockSize * dim);
            for (int i = 0; i < blockSize; i++) {
                x[i] = static_cast<int>(tmppts[i]);
            }
            for (int i = 0; i < blockSize; i++) {
                y[i] = static_cast<int>(tmppts[i + blockSize]);
            }
            if (dim == 3) {
                for (int i = 0; i < blockSize; i++) {
                    z[i] = static_cast<int>(tmppts[i + 2 * blockSize]);
                }
            }
            delete[] tmppts;
        }
    } else {
        for (int ii = 0; ii < blockSize; ii++) {
            fseek(curOpenMeshFile->file(), (long)curOpenMeshFile->iAsciiFileStart +
                (long)block * curOpenMeshFile->iAsciiFileLineLen * blockSize +
                (long)ii * curOpenMeshFile->iAsciiFileLineLen, SEEK_SET);
            if (dim == 3) {
                int res = fscanf(curOpenMeshFile->file(), " %f %f %f", &x[ii], &y[ii], &z[ii]); (void)res;
            } else {
                int res = fscanf(curOpenMeshFile->file(), " %f %f", &x[ii], &y[ii]); (void)res;
                memset(z, 0, blockSize * sizeof(float));
            }
        }
    }
    //store corners
    vector < array<float, 3>> corners;

    return true;
}

bool PartitionReader::ReadVelocity(int timestep, int block, float* x, float* y, float* z) {
    int fileID = getFileID(block);
    if(!CheckOpenFile(curOpenVarFile, timestep, fileID))
        return false;

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, "velocity");
    if (isParalellFormat)
        block = blockMap[block].second;
    //block = myBlockPositions[block];

    long iRealHeaderSize = iHeaderSize + (isParalellFormat ? vBlocksPerFile[fileID] * sizeof(int) : 0);

    if (isBinary) {
        long filepos;
        if (!isParalellFormat)
            filepos = (long)iRealHeaderSize + (long)(dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else
            //This assumes [block 0: 216u 216v 216w][block 1: 216u 216v 216w]...[block n: 216u 216v 216w]
            filepos = (long)iRealHeaderSize +
            (long)vBlocksPerFile[fileID] * dp.varOffsetBinary * iPrecision + //the header and mesh if one exists
            (long)block * blockSize * dim * iPrecision;
        if (iPrecision == 4) {
            fseek(curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(x, sizeof(float), blockSize, curOpenVarFile->file()); (void)res;
            res = fread(y, sizeof(float), blockSize, curOpenVarFile->file()); (void)res;
            if (dim == 3) {
                res = fread(z, sizeof(float), blockSize, curOpenVarFile->file()); (void)res;
            }
            else {
                memset(z, 0, blockSize * sizeof(float));
            }
            //for (size_t i = 0; i < blockSize; i++) {
            //    float v = sqrt()
            //}
            if (bSwapEndian) {
                ByteSwapArray(x, blockSize);
                ByteSwapArray(y, blockSize);
                if (dim == 3) {
                    ByteSwapArray(z, blockSize);
                }
            }
        } else {
            double* tmppts = new double[blockSize * dim];
            fseek(curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), blockSize * dim, curOpenVarFile->file()); (void)res;

            if (bSwapEndian)
                ByteSwapArray(tmppts, blockSize * dim);

            for (int ii = 0; ii < blockSize; ii++) {
                x[ii] = (double)tmppts[ii];
                y[ii] = (double)tmppts[ii + blockSize];
                if (dim == 3) {
                    z[ii] = (double)tmppts[ii + blockSize + blockSize];
                } else {
                    z[ii] = 0.0;
                }
            }
            delete[] tmppts;
        }
    } else {
        for (int ii = 0; ii < blockSize; ii++) {
            fseek(curOpenVarFile->file(), (long)curOpenVarFile->iAsciiFileStart +
                (long)block * curOpenVarFile->iAsciiFileLineLen * blockSize +
                (long)ii * curOpenVarFile->iAsciiFileLineLen +
                (long)dp.varOffsetAscii, SEEK_SET);
            if (dim == 3) {
                int res = fscanf(curOpenVarFile->file(), " %f %f %f", x + ii, y +ii, z+ ii); (void)res;
            } else {
                int res = fscanf(curOpenVarFile->file(), " %f %f", &x[ii], &y[ii]); (void)res;
                z[ii] = 0.0f;
            }
        }
    }
    return true;
}

bool PartitionReader::ReadVar(const string &varname, int timestep, int block, float* data) {

    int fileID = getFileID(block);
    if(!CheckOpenFile(curOpenVarFile, timestep, fileID))
        return false;

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, varname);

    if (isParalellFormat)
        block = blockMap[block].second;
    //block = myBlockPositions[block];

    long iRealHeaderSize = iHeaderSize + (isParalellFormat ? vBlocksPerFile[fileID] * sizeof(int) : 0);

    if (isBinary) {
        long filepos;
        if (!isParalellFormat)
            filepos = (long)iRealHeaderSize + ((long)dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else {
            // This assumes uvw for all fields comes after the mesh as [block0: 216u 216v 216w]...
            // then p or t as   [block0: 216p][block1: 216p][block2: 216p]...
            if (strcmp(varname.c_str() + 2, "velocity") == 0) {
                filepos = (long)iRealHeaderSize +                              //header
                    (long)dp.timestepHasMesh * vBlocksPerFile[fileID] * blockSize * dim * iPrecision + //mesh
                    (long)block * blockSize * dim * iPrecision +                  //start of block
                    (long)(varname[0] - 'x') * blockSize * iPrecision;            //position within block
            } else
                filepos = (long)iRealHeaderSize +
                (long)vBlocksPerFile[fileID] * dp.varOffsetBinary * iPrecision + //the header, mesh, vel if present,
                (long)block * blockSize * iPrecision;
        }
        if (iPrecision == 4) {
            fseek(curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(data, sizeof(float), blockSize, curOpenVarFile->file()); (void)res;
            if (bSwapEndian)
                ByteSwapArray(data, blockSize);
        } else {
            double* tmp = new double[blockSize];

            fseek(curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(tmp, sizeof(double), blockSize, curOpenVarFile->file()); (void)res;
            if (bSwapEndian)
                ByteSwapArray(tmp, blockSize);

            for (int ii = 0; ii < blockSize; ii++)
                data[ii] = (float)tmp[ii];

            delete[] tmp;
        }
    } else {
        float* var_tmp = data;
        for (int ii = 0; ii < blockSize; ii++) {
            fseek(curOpenVarFile->file(), (long)curOpenVarFile->iAsciiFileStart +
                (long)block * curOpenVarFile->iAsciiFileLineLen * blockSize +
                (long)ii * curOpenVarFile->iAsciiFileLineLen +
                (long)dp.varOffsetAscii, SEEK_SET);
            int res = fscanf(curOpenVarFile->file(), " %f", var_tmp); (void)res;
            var_tmp++;
        }
    }
    return true;
}

bool PartitionReader::ReadBlockLocations() {
    // In each parallel file, in the header, there's a table that maps
    // each local block to a global id which starts at 1.  Here, I make
    // an inverse map, from a zero-based global id to a proc num and local
    // offset.

    if (!isBinary || !isParalellFormat)
        return true;

    vBlocksPerFile = std::vector<int>(numOutputDirs);
    std::fill(vBlocksPerFile.begin(), vBlocksPerFile.end(), 0);


    string blockfilename;
    std::vector<int> tmpBlocks(totalNumBlocks);
    ifstream f;
    int badFile = totalNumBlocks + 1;
    for (int ii = 0; ii < numOutputDirs; ++ii) {
        blockfilename = GetFileName(0, ii);
        f.open(blockfilename, ifstream::binary);
        if (!f.is_open()) {
            badFile = ii;
            break;
        }

        int tmp1, tmp2, tmp3, tmp4;
        f.seekg(5, std::ios_base::beg);  //seek past the #std
        f >> tmp1 >> tmp2 >> tmp3 >> tmp4 >> vBlocksPerFile[ii];

        f.seekg(136, std::ios_base::beg);
        f.read((char*)tmpBlocks.data()+ ii * vBlocksPerFile[ii], vBlocksPerFile[ii] * sizeof(int));
        if (bSwapEndian)
            ByteSwapArray(tmpBlocks.data() + ii * vBlocksPerFile[ii], vBlocksPerFile[ii]);


        f.close();

        for (int jj = ii * vBlocksPerFile[ii]; jj < (ii + 1) * vBlocksPerFile[ii]; ++jj) {
            int iBlockID = tmpBlocks[jj] - 1;
            if (iBlockID < 0 ||
                iBlockID >= totalNumBlocks) {
                sendError(" .nek500: Error reading parallel file block IDs.");
                return false;
            }
                    blockMap[iBlockID] = std::make_pair(ii, jj);
        }
    }

    //badFile = UnifyMinimumValue(badFile);
    if (badFile < totalNumBlocks) {
        blockfilename = GetFileName(0, badFile);
        sendError("Could not open file " + blockfilename + " to read block locations." );
        return false;
    }

    //Do a sanity check
    int sum = 0;

    for (int ii = 0; ii < numOutputDirs; ii++)
        sum += vBlocksPerFile[ii];

    if (sum != totalNumBlocks) {
        sendError("nek5000: Sum of blocks per file does not equal total number of blocks");
        return false;
    }
    return true;

}

void PartitionReader::FindAsciiDataStart(FILE* fd, int& outDataStart, int& outLineLen) {
    //Skip the header, then read a float for each block.  Then skip beyond the
//newline character and return the current position.
    fseek(fd, iHeaderSize, SEEK_SET);
    for (int ii = 0; ii < totalNumBlocks; ii++) {
        float dummy;
        int res = fscanf(fd, " %f", &dummy); (void)res;
    }
    char tmp[1024];
    char* res = nullptr; (void)res;
    res = fgets(tmp, 1023, fd);
    outDataStart = ftell(fd);

    res = fgets(tmp, 1023, fd);
    outLineLen = ftell(fd) - outDataStart;
}

PartitionReader::DomainParams PartitionReader::GetDomainSizeAndVarOffset(int iTimestep, const string& var) {
    DomainParams params;
    params.timestepHasMesh = 0;

    UpdateCyclesAndTimes();   //Needs to call this to update iTimestepsWithMesh

    if (vTimestepsWithMesh[iTimestep] == true)
        params.timestepHasMesh = 1;

    int nFloatsPerSample = 0;
    if (params.timestepHasMesh)
        nFloatsPerSample += dim;
    if (bHasVelocity)
        nFloatsPerSample += dim;
    if (bHasPressure)
        nFloatsPerSample += 1;
    if (bHasTemperature)
        nFloatsPerSample += 1;
    nFloatsPerSample += numScalarFields;

    params.domSizeInFloats = nFloatsPerSample * blockSize;

    if (var.length() > 0) {
        int iNumPrecedingFloats = 0;
        if (var == "velocity"||
            var == "velocity_mag"||
            var == "x_velocity") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += dim;
        } else if (var == "y_velocity") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += dim;

            iNumPrecedingFloats += 1;
        } else if (var == "z_velocity") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += dim;

            iNumPrecedingFloats += 2;
        } else if (var == "pressure") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += dim;
            if (bHasVelocity)
                iNumPrecedingFloats += dim;
        } else if (var == "temperature") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += dim;
            if (bHasVelocity)
                iNumPrecedingFloats += dim;
            if (bHasPressure)
                iNumPrecedingFloats += 1;
        } else if (var[0] == 's') {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += dim;
            if (bHasVelocity)
                iNumPrecedingFloats += dim;
            if (bHasPressure)
                iNumPrecedingFloats += 1;
            if (bHasTemperature)
                iNumPrecedingFloats += 1;

            int iSField = atoi(var.c_str() + 1);
            //iSField should be between 1..iNumSFields, inclusive
            iNumPrecedingFloats += iSField - 1;
        }
        params.varOffsetBinary = blockSize * iNumPrecedingFloats;
        params.varOffsetAscii = 14 * iNumPrecedingFloats;
    }
    return params;
}

int PartitionReader::getFileID(int block)
{
    int fileID = 0;
    if(isParalellFormat)
        fileID = blockMap[block].first;
    return fileID;
}

bool PartitionReader::CheckOpenFile(std::unique_ptr<OpenFile>& file, int timestep, int fileID)
{
    if (timestep < 0) {
        return false;
    }
    string filename = GetFileName(timestep, fileID);

    if(!file || file->name() != filename)
    {
        file.reset(new OpenFile(filename));
        if(!file->file())
            return false;
        if (!isBinary)
            FindAsciiDataStart(file->file(), file->iAsciiFileStart, file->iAsciiFileLineLen);
    }
    return true;
}

void PartitionReader::addGhostBlocks(std::vector<int>& blocksNotToRead, int numGhostLayers) {
    if (myBlocksToRead.size() == numBlocksToRead) {
        return;
    }
    numGhostBlocks = 0;
    for (size_t iterations = 0; iterations < numGhostLayers; iterations++) {
    int oldNumBlocks = myBlocksToRead.size();
    for (int myBlock = 0; myBlock < oldNumBlocks;++myBlock) {
            for (auto i = mapFileData[myBlocksToRead[myBlock]].begin() + 1; i < mapFileData[myBlocksToRead[myBlock]].end(); i++) {
                {
                    auto notMyBlock = blocksNotToRead.begin();
                    while (notMyBlock != blocksNotToRead.end()) {
                        if (find(mapFileData[*notMyBlock].begin() + 1, mapFileData[*notMyBlock].end(), *i) != mapFileData[*notMyBlock].end()) {
                            myBlocksToRead.push_back(*notMyBlock);
                            notMyBlock = blocksNotToRead.erase(notMyBlock);
                        }
                        else {
                            ++notMyBlock;
                        }
                    }
                }
            }
        }
        numGhostBlocks += myBlocksToRead.size() -  oldNumBlocks;
    }
}

bool PartitionReader::makeConnectivityList() {

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
    map<Edge, pair<bool, vector<int>>> writtenEdges; //key is sorted global indices, bool is wehter it had to be sorted, vector contains the indices to the coordinates in mesh file
    map<Plane, pair< Plane, vector<int>>> writtenPlanes; //key is sorted global indices, pair contains unsorted globl indices and a vector containing the indices to the coordinates in mesh file

    connectivityList.clear();
    connectivityList.reserve(getNumConn());
    gridSize = blockSize * myBlocksToRead.size();
    int numDoublePoints = 0;


    for (size_t i = 0; i < 3; i++) {
        myGrid[i].resize(myBlocksToRead.size() * blockSize);
    }
    for (size_t currBlock = 0; currBlock < myBlocksToRead.size(); currBlock++) {
        //preRead the grid to verify overlapping corners. The is necessary beacause the mapfile also contains not physically (logically) linked blocks
        array<vector<float>, 3> grid{ vector<float>(blockSize), vector<float>(blockSize) , vector<float>(blockSize) };
        if (!ReadMesh(timestepToUseForMesh, myBlocksToRead[currBlock], grid[0].data(), grid[1].data(), grid[2].data())) {
            return false;
        }
        //contains the points, that are already written(in local block indices) and where to find them in the coordinate list and a set of new edges that have to be reversed
        map<int, int> localToGloabl = findAlreadyWrittenPoints(currBlock, writtenCorners, writtenEdges, writtenPlanes, grid);

        fillConnectivityList(localToGloabl, currBlock * blockSize - numDoublePoints);
        numDoublePoints += localToGloabl.size();

        //add my corners/edges/planes to the "already written" maps
        addNewCorners(writtenCorners, currBlock);
        addNewEdges(writtenEdges, currBlock);
        if (dim == 3) {
            addNewPlanes(writtenPlanes, currBlock);
        }
        //add the new connectivityList entries to my grid
        int numCon = numCorners * hexesPerBlock;
        for (size_t i = 0; i < numCon; i++) {
            for (size_t j = 0; j < 3; j++) {
                myGrid[j][connectivityList[i + currBlock * numCon]] = grid[j][connectivityList[i]];
            }
        }
    }
    gridSize -= numDoublePoints;
    //cerr << "______connectivity list ______" << endl;
    //for (size_t i = 0; i < connectivityList.size(); i++) {
    //    cerr << "[" << i << "] " << connectivityList[i] << "  ";
    //    if (i % 6 == 0) {
    //        cerr << endl;
    //    }
    //}

    return true;
}

bool PartitionReader::checkPointInGridGrid(const array<vector<float>, 3> &currGrid, int currGridIndex, int gridIndex)     {
    for (size_t i = 0; i < 3; i++) {
        float diff = currGrid[i][currGridIndex] - myGrid[i][gridIndex];
        if (diff * diff > 0.00001) {
            return false;
        }
    }
    return true;
}

std::vector<int> PartitionReader::getMatchingPlanePoints(int block, const Plane& plane, const Plane& globalWrittenPlane) {
//find edges that form a plane
    block = myBlocksToRead[block];
    Plane writtenPlane;
    for (size_t i = 0; i < 4; i++) {
        writtenPlane[i] = find(mapFileData[block].begin() + 1, mapFileData[block].end(), globalWrittenPlane[i]) - mapFileData[block].begin();
    }
    vector<vector<int>> corners{ vector<int>{plane[0], plane[1]},vector<int>{plane[2], plane[3]} };
    vector<vector<int>> writtenCorners{ vector<int>{writtenPlane[0], writtenPlane[1]},vector<int>{writtenPlane[2], writtenPlane[3]} };
    std::array<bool, 3> invertedAxes{ false, false, false };
    if (corners != writtenCorners) {
        if (plane[3] - plane[0] == 3) {//xy plane
            if (writtenCorners[0] == corners[1]) {//y inverted
                invertedAxes[0] = true;
            }
            else {//x inverted
                invertedAxes[1] = true;
            }
        }
        if (plane[3] - plane[0] == 5) {//xz plane
            if (writtenCorners[0] == corners[1]) {//x inverted
                invertedAxes[2] = true;
            } else {//z inverted
                invertedAxes[0] = true;
            }
        }
        if (plane[3] - plane[0] == 6) {//yz plane
            if (writtenCorners[0] == corners[1]) {//y inverted
                invertedAxes[2] = true;
            } else {//z inverted
                invertedAxes[1] = true;
            }
        }
    }
    vector<int> p(4);
    copy(plane.begin(), plane.end(), p.begin());

    return getIndicesBetweenCorners(p, invertedAxes);;
}

std::map<int, int> PartitionReader::findAlreadyWrittenPoints(const size_t& currBlock,
    const std::map<int, int>& writtenCorners,
    const map<Edge, pair<bool, vector<int>>> writtenEdges,
    const map<Plane, pair< Plane, vector<int>>>& writtenPlanes,
    const std::array<std::vector<float>, 3> & currGrid) {
    
    std::map<int, int> localToGloabl;
    map<Edge, vector<int>> myWrittenEdges; //the already written edges of this block and their indices in the grid
    auto corners = mapFileData[myBlocksToRead[currBlock]]; //corner info starts at index 1
    //Lookup if a corner of this block has already been written
    for (size_t i = 1; i < corners.size(); i++) {
        auto ci = writtenCorners.find(corners[i]);
        int blockIndex = cornerIndexToBlockIndex(i);
        if (ci != writtenCorners.end() && checkPointInGridGrid(currGrid, blockIndex, ci->second)) { //corner already written
            localToGloabl[blockIndex] = ci->second;
        }
    }
    if (localToGloabl.size() > 1) {
        //check for edges
        for (Edge edge : allEdgesInCornerIndices) {
            Edge globalEdge;
            for (size_t i = 0; i < 2; i++) {
                globalEdge[i] = corners[edge[i]];
            }
            auto unsortedEdge = globalEdge;
            sort(globalEdge.begin(), globalEdge.end());
            bool sorted = unsortedEdge == globalEdge ? false : true;
            


            auto it = writtenEdges.find(globalEdge);
            if (it != writtenEdges.end()) {
                auto e = myWrittenEdges.insert({ edge, it->second.second }).first;
                if (it->second.first != sorted) {//keep the points sorted
                    reverse(e->second.begin(), e->second.end());
                }
            }
        }
    }
    ////detect which of the new edges of this block must be reversed
    //for (auto edge : myWrittenEdges)         {
    //    if (edge.second) {
    //        for(auto edge2 : myNewEdges)               {
    //            if (edge.first[1] - edge.first[0] == edge2[1] - edge2[0]) {
    //                myNewReversedEdges.insert(edge2);
    //            }
    //        }
    //    }
    //}
    
    //check and insert planes before eges are inserted because edges may be in a reversed order
    if (dim == 3 && myWrittenEdges.size() > 3) {
        //check for planes
        for (Plane plane : allPlanesInCornerIndices)             {
            Plane globalPlane;
            vector<int> localPlane(4);
            for (size_t i = 0; i < 4; i++) {
                globalPlane[i] = corners[plane[i]];
                localPlane[i] = plane[i];
            }
            auto unsortedPlane = globalPlane;
            sort(globalPlane.begin(), globalPlane.end());
            auto it = writtenPlanes.find(globalPlane);
            if (it != writtenPlanes.end()) {
                auto globalPoints =it->second.second;
                auto localPoints = getMatchingPlanePoints(currBlock, plane, it->second.first);
                for (size_t i = 0; i < globalPoints.size(); i++) {
                    if (checkPointInGridGrid(currGrid, localPoints[i], globalPoints[i])) {
                        localToGloabl[localPoints[i]] = globalPoints[i];
                    }
                }
            }
        }

    }
    for (auto edge : myWrittenEdges)         {
        auto localPoints = getIndicesBetweenCorners({ edge.first[0],edge.first[1] });
        for (size_t i = 0; i < edge.second.size(); i++) {
            if (checkPointInGridGrid(currGrid, localPoints[i], edge.second[i])) {
                localToGloabl[localPoints[i]] = edge.second[i];
            }
        }
    }

    return localToGloabl;
}

vector<int> PartitionReader::getIndicesBetweenCorners(std::vector<int> corners, const std::array<bool, 3> & invertAxis)     {
    if (corners.size() == 1) {
        return vector<int>{cornerIndexToBlockIndex(corners[0])};
    }
    if (corners.size() == 2) {
        vector<int> indices;
        int myDim = blockDimensions[0] - 1;
        if (corners[1] - corners[0] == 2) {
            myDim = blockDimensions[1] - 1;
        }
        else if(corners[1] - corners[0] == 4){
            myDim = blockDimensions[2] - 1;
        }

        int ci1 = cornerIndexToBlockIndex(corners[0]);
        int ci2 = cornerIndexToBlockIndex(corners[1]);
        int step = (ci2 - ci1) / myDim;
        for (size_t i = ci1; i <= ci2; i+= step) {
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
                for (int i = 0; i < blockDimensions[1]; i++) {
                    vector<int> inv;
                    inv.reserve(blockDimensions[0]);
                    for (int j = blockDimensions[0] - 1; j >= 0; --j) {
                        inv.push_back(indices[i * blockDimensions[0] + j]);
                    }
                    copy(inv.begin(), inv.end(), indices.begin() + i * blockDimensions[0]);
                }
            }
            if (invertAxis[1]) {
                int dim1 = blockDimensions[0];
                int dim2 = blockDimensions[1];
                for (int i = 0; i <dim2 / 2; i++) {
                    vector<int> tmp(dim1);
                    copy(indices.begin() + i *dim1, indices.begin() + (i + 1) *dim1, tmp.begin());
                    copy(indices.end() - (i + 1) *dim1, indices.end() - i *dim1, indices.begin() + i * dim1);
                    copy(tmp.begin(), tmp.end(), indices.end() - (i + 1) *dim1);
                }
            }
        }
        else if (corners[3] - corners[0] == 5) { //xz plane
            int start = cornerIndexToBlockIndex(corners[0]);
            for (int i = 0; i < blockDimensions[2]; i++) {
                for (int j = 0; j < blockDimensions[1]; j++) {
                    indices.push_back(start + j);
                }
                start += blockDimensions[0] * blockDimensions[1];
            }
            if (invertAxis[0]) {
                for (int i = 0; i < blockDimensions[2]; i++) {
                    vector<int> inv;
                    inv.reserve(blockDimensions[0]);
                    for (int j = blockDimensions[0] - 1; j >= 0; --j) {
                        inv.push_back(indices[i * blockDimensions[0] + j]);
                    }
                    copy(inv.begin(), inv.end(), indices.begin() + i * blockDimensions[0]);
                }
            }
            if (invertAxis[2]) {
                int dim1 = blockDimensions[0];
                int dim2 = blockDimensions[2];
                for (int i = 0; i < dim2 / 2; i++) {
                    vector<int> tmp(dim1);
                    copy(indices.begin() + i * dim1, indices.begin() + (i + 1) * dim1, tmp.begin());
                    copy(indices.end() - (i + 1) * dim1, indices.end() - i * dim1, indices.begin() + i * dim1);
                    copy(tmp.begin(), tmp.end(), indices.end() - (i + 1) * dim1);
                }
            }
        }
        else if (corners[3] - corners[0] == 6) {//yz plane
            for (int i = cornerIndexToBlockIndex(corners[0]); i <= cornerIndexToBlockIndex(corners[3]); i += blockDimensions[0]) {
                indices.push_back(i);
            }
            if (invertAxis[1]) {
                for (int i = 0; i < blockDimensions[2]; i++) {
                    vector<int> inv;
                    inv.reserve(blockDimensions[1]);
                    for (int j = blockDimensions[1] - 1; j >= 0; j--) {
                        inv.push_back(indices[i * blockDimensions[1] + j]);
                    }
                    copy(inv.begin(), inv.end(), indices.begin() + i * blockDimensions[1]);
                }
            }
            if (invertAxis[2]) {
                int dim1 = blockDimensions[1];
                int dim2 = blockDimensions[2];
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

int PartitionReader::getLocalBlockIndex(int x, int y, int z)     {
    int index = x;
    index += y * blockDimensions[0];
    index += z * blockDimensions[0] * blockDimensions[1];
    return index;

}

int PartitionReader::cornerIndexToBlockIndex(int cornerIndex)     {
    switch (cornerIndex) {
    case (1):
        return getLocalBlockIndex(0, 0, 0);
    case (2):
        return getLocalBlockIndex(blockDimensions[0] - 1, 0, 0);
    case (3):
        return getLocalBlockIndex(0, blockDimensions[1] - 1, 0);
    case (4):
        return getLocalBlockIndex(blockDimensions[0] - 1, blockDimensions[1] - 1, 0);
    case (5):
        return getLocalBlockIndex(0, 0, blockDimensions[2] - 1);
    case (6):
        return getLocalBlockIndex(blockDimensions[0] - 1, 0, blockDimensions[2] - 1);
    case (7):
        return getLocalBlockIndex(0, blockDimensions[1] - 1, blockDimensions[2] - 1);
    case (8):
        return getLocalBlockIndex(blockDimensions[0] - 1, blockDimensions[1] - 1, blockDimensions[2] - 1);
    default:
        break;
    }
}

void PartitionReader::fillConnectivityList(const map<int, int>& localToGloabl, int startIndexInMesh) {

    vector<vistle::Index> connList(numCorners * hexesPerBlock);
    int numDoubles = 0;

    for (size_t index = 0; index < blockSize; index++) {
        auto conListEntries = blockIndexToConnectivityIndex.find(index)->second;
        bool added = false;
        auto it = localToGloabl.find(index);
        if (it != localToGloabl.end()) {
            for (auto entry : conListEntries) {
                connList[entry] = it->second;
            }
            ++numDoubles;
            added = true;
        }
        if (!added) {
            for (auto entry : conListEntries) {
                connList[entry] = startIndexInMesh + index - numDoubles;
            }
        }
    }
    connectivityList.insert(connectivityList.end(), connList.begin(), connList.end());
}

void PartitionReader::addNewCorners(map<int, int> &allCorners, int localBlock)     {
    auto corners = mapFileData[myBlocksToRead[localBlock]];
    for (size_t i = 1; i <= numCorners; i++) {
        allCorners[corners[i]] = connectivityList[blockIndexToConnectivityIndex.find(cornerIndexToBlockIndex(i))->second[0] + localBlock * numCorners * hexesPerBlock];
    }
}

void PartitionReader::addNewEdges(map<Edge, pair<bool, vector<int>>>& allEdges, int localBlock) {
    
    auto corners = mapFileData[myBlocksToRead[localBlock]];
    for (auto edge : allEdgesInCornerIndices) {

        Edge e = { corners[edge[0]], corners[edge[1]] };
        auto unsortedE = e;
        sort(e.begin(), e.end());
        bool s = unsortedE == e ? false : true;

        auto it = allEdges.find(e);
        if (it == allEdges.end()) {
            vector<int> blockIndices = getIndicesBetweenCorners({ edge[0], edge[1] });
            vector<int> gridIndices;
            gridIndices.reserve(blockIndices.size());
            for (int blockIndex : blockIndices) {
                gridIndices.push_back(connectivityList[blockIndexToConnectivityIndex.find(blockIndex)->second[0] + localBlock * numCorners * hexesPerBlock]);
            }
            allEdges[e] = make_pair(s, gridIndices);
        }
    }
}

void PartitionReader::addNewPlanes(map<Plane, pair< Plane, vector<int>>>& allPlanes, int localBlock) {

    auto corners = mapFileData[myBlocksToRead[localBlock]];
    for (auto plane : allPlanesInCornerIndices) {
        vector<int> blockIndices = getIndicesBetweenCorners({ plane[0], plane[1], plane[2], plane[3] });
        vector<int> gridIndices;
        gridIndices.reserve(blockIndices.size());
        for (int blockIndex : blockIndices) {
            gridIndices.push_back(connectivityList[blockIndexToConnectivityIndex.find(blockIndex)->second[0] + localBlock * numCorners * hexesPerBlock]);
        }


        Plane e = { corners[plane[0]], corners[plane[1]], corners[plane[2]], corners[plane[3]] };
        auto unsortedE = e;
        sort(e.begin(), e.end());
        allPlanes[e] = make_pair(unsortedE, gridIndices);
    }
}

}

