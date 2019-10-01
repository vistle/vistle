#include "PartitionReader.h"
#include <set>
//#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <array>

using namespace std;

namespace nek5000 {
//public methods
PartitionReader::PartitionReader(ReaderBase &base)
    :ReaderBase((base))
{
}




bool PartitionReader::fillMesh(float *x, float *y, float *z)
{
    for (size_t b = 0; b < myBlocksToRead.size(); b++) {
        array<vector<float>, 3> grid{ vector<float> (blockSize), vector<float>(blockSize) , vector<float>(blockSize) };
        if (!ReadMesh(timestepToUseForMesh, myBlocksToRead[b], grid[0].data(), grid[1].data(), grid[2].data())) {
            return false;
        }
        int numCon = numCorners * hexesPerBlock;
        for (size_t i = 0; i < numCon; i++) {

            x[connectivityList[i + b * numCon]] = grid[0][connectivityList[i]];
            y[connectivityList[i + b * numCon]] = grid[1][connectivityList[i]];
            z[connectivityList[i + b * numCon]] = grid[2][connectivityList[i]];
        }

    }
    return true;
}

bool PartitionReader::fillVelocity(int timestep, float *x, float *y, float *z)
{
    if (bHasVelocity) {
        int numCon = numCorners * hexesPerBlock;
        for (size_t b = 0; b < myBlocksToRead.size(); b++) {
            array<vector<float>, 3> velocity{ vector<float>(blockSize), vector<float>(blockSize) , vector<float>(blockSize) };
            if (!ReadVelocity(timestep, myBlocksToRead[b], velocity[0].data(), velocity[1].data(), velocity[2].data())) {
                return false;
            }
            for (size_t i = 0; i < numCon; i++) {

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
        fill(scalar.begin(), scalar.end(), b % (blockSize / 10));
        //if (!ReadVar(varName, timestep, myBlocksToRead[b], scalar.data())) {
        //    return false;
        //}
        for (size_t i = 0; i < numCon; i++) {

            data[connectivityList[i + b * numCon]] = scalar[connectivityList[i]];
        }
    }
    return true;
}


//getter
size_t PartitionReader::getBlocksToRead() const
{
    return myBlocksToRead.size();
}
size_t PartitionReader::getFirstBlockToRead() const
{
    return 0;
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

void PartitionReader::getConnectivityList(vistle::Index* connList) {
    memcpy(connList, connectivityList.data(), connectivityList.size() * sizeof(vistle::Index));
}




//setter
bool PartitionReader::setPartition(int partition, bool useMap)
{
    if(partition > numPartitions)
        return false;
    myPartition = partition;
    int lower = mapFileHeader[3] / numPartitions * partition; //toDo: what if thre is a rest?
    int upper = lower + mapFileHeader[3] / numPartitions;

    int num = numBlocksToRead / numPartitions;
    int oneMore = numBlocksToRead % numPartitions;
    num += partition < oneMore ? 1 : 0;
    for (size_t i = 0; i < num; i++) { //may not work if not all blocks to read on multiple partitions
        if (mapFileData[i][0] >= lower && mapFileData[i][0] < upper) {
            myBlocksToRead.push_back(i);
        }
    }
    if(!ReadBlockLocations(useMap))
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

    array<int, 8> cornerIndex;
    cornerIndex[0] = 0;//0,0,0
    cornerIndex[1] = blockDimensions[0] - 1;//1,0,0
    cornerIndex[2] = blockDimensions[0] * (blockDimensions[1] - 1);//0,1,0
    cornerIndex[3] = cornerIndex[1] + cornerIndex[2];//1,1,0
    cornerIndex[4] = blockDimensions[0] * blockDimensions[1] * (blockDimensions[2] - 1);// 0, 0, 1
    cornerIndex[5] = cornerIndex[1] + cornerIndex[4]; //1,0,1
    cornerIndex[6] = cornerIndex[2] + cornerIndex[4]; //0,1,1
    cornerIndex[7] = cornerIndex[1] + cornerIndex[6]; //1,1,1
    for (size_t i = 0; i < 8; i++) {
        corners.push_back(array<float, 3>{*(x + cornerIndex[i]), * (y + cornerIndex[i]), * (z + cornerIndex[i])});
    }
    blockCorners[block] = corners;
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

std::vector<std::vector<float>> PartitionReader::ReadBlock(int block) {
    std::vector<std::vector<float>> blockData;
    int numData = dim + dim * bHasVelocity ? 1 : 0 + bHasPressure ? 1 : 0 + bHasTemperature ? 1 : 0 + numScalarFields;
    for (size_t i = 0; i < numData; i++) {

    }
    return blockData;
}

bool PartitionReader::ReadBlockLocations(bool useMap) {
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
    bool fromMap = true;
    if(!useMap)
    {
        all_element_list = tmpBlocks;
        fromMap = false;
    }
    myBlockIDs.clear();
    myBlockIDs.reserve(myBlocksToRead.size());
    for(int i=0; i < myBlocksToRead.size(); ++i)
    {
      myBlockIDs.emplace_back(all_element_list[0 + i]);
    }
    if(fromMap)
    {
        std::sort(myBlockIDs.begin(), myBlockIDs.end());
    }
    myBlockPositions.clear();
    myBlockPositions.reserve(myBlocksToRead.size());
    for(int i=0; i < myBlocksToRead.size(); ++i)
    {
        auto pos = blockMap.find(myBlockIDs[i] - 1);
        if (pos != blockMap.end())             {
            myBlockPositions.emplace_back(pos->second.second);
        }
        else {
            cerr << "error" << endl;
        }

    }

    // TEMP: checking for duplicates within myBlockPositions
    if(fromMap)
    {
      for(int i=0; i<this->myBlocksToRead.size() -1; i++)
      {
        for(int j=i+1; j<this->myBlocksToRead.size(); j++)
        {
          if(this->myBlockPositions[i] == this->myBlockPositions[j])
          {
            sendError("nek5000: my_partition: " + to_string(myPartition) + " : Hey (this->myBlockPositions[" + to_string(i) + "] and [" + to_string(j) + "] both == " + to_string(this->myBlockPositions[j]));
          }
        }
      }
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

void PartitionReader::makeConnectivityList() {

    //maps from  (sorted) global (from map file) cornerid(s) to cornerindex(indices) in connectivity list
    
    map<int, int> writtenCorners;
    map<array<int, 2>, vector<int>> writtenEdges; //sort vector by corner id from map file
    map<array<int, 4>, vector<int>> writtenPlanes; //sort vector by corner id from map file

    connectivityList.clear();
    connectivityList.reserve(getNumConn());
    gridSize = blockSize * myBlocksToRead.size();
    int numDoublePoints = 0;
    //construct a map from blockIndex to a corresponding index in the connectivity list
    constructBlockIndexToConnectivityIndex();
    //process other blocks
    for (size_t currBlock = 0; currBlock < myBlocksToRead.size(); currBlock++) {
        //contains the points, that are already written(in local block indices) and where to find them in the coordinate list
        map<int, int> localToGloabl = findAlreadyWrittenPoints(currBlock, writtenCorners, writtenEdges, writtenPlanes);
        fillConnectivityList3(localToGloabl, currBlock * blockSize - numDoublePoints);
        numDoublePoints += localToGloabl.size();

        //add my corners/edges/planes to the "already written" maps
        addNewCorners(writtenCorners, currBlock);
        addNewEdges(writtenEdges, currBlock);
        if (dim == 3) {
            addNewPlanes(writtenPlanes, currBlock);
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


}

std::map<int, int> PartitionReader::findAlreadyWrittenPoints(const size_t& currBlock, const std::map<int, int>& writtenCorners, std::map < std::array<int, 2>, std::vector<int>>& writtenEdges, std::map < std::array<int, 4>, std::vector<int>>& writtenPlanes) {
    std::map<int, int> localToGloabl;
    int numWrittenEdges = 0;
    auto corners = mapFileData[myBlocksToRead[currBlock]]; //corner info starts at index 1
    //Lookup if a corner of this block has already been written
    for (size_t i = 1; i < corners.size(); i++) {
        auto ci = writtenCorners.find(corners[i]);
        if (ci != writtenCorners.end()) { //corner already written
            localToGloabl[cornerIndexToBlockIndex(i)] = ci->second;
        }
    }
    if (localToGloabl.size() > 1) {
        //check for edges
        auto allEdges = getAllEdgesInCornerIndices();
        for (array<int, 2> edge : allEdges) {
            array<int, 2> globalEdge;
            vector<int> localEdge(2);
            for (size_t i = 0; i < 2; i++) {
                globalEdge[i] = corners[edge[i]];
                localEdge[i] = edge[i];
            }
            sort(globalEdge.begin(), globalEdge.end());
            auto it = writtenEdges.find(globalEdge);
            if (it != writtenEdges.end()) {
                auto globalPoints = it->second;
                auto localPoints = getIndicesBetweenCorners(localEdge);
                for (size_t i = 0; i < globalPoints.size(); i++) {
                    localToGloabl[localPoints[i]] = globalPoints[i];
                }
                ++numWrittenEdges;
            }
        }
    }
    if (dim == 3 && numWrittenEdges > 3) {
        //check for planes
        auto allPlanes = getAllPlanesInCornerIndices();
        for (array<int, 4> plane : allPlanes)             {
            array<int, 4> globalPlane;
            vector<int> localPlane(4);
            for (size_t i = 0; i < 4; i++) {
                globalPlane[i] = corners[plane[i]];
                localPlane[i] = plane[i];
            }
            sort(globalPlane.begin(), globalPlane.end());
            auto it = writtenPlanes.find(globalPlane);
            if (it != writtenPlanes.end()) {
                auto globalPoints =it->second;
                auto localPoints = getIndicesBetweenCorners(localPlane);
                for (size_t i = 0; i < globalPoints.size(); i++) {
                    localToGloabl[localPoints[i]] = globalPoints[i];
                }
            }
        }

    }
    return localToGloabl;
}

void PartitionReader::addToConnectivityList(int index, int start, int xx, int yy, int zz)     {
    switch (index) {
    case 1:
        connectivityList.push_back(zz * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + start);
        break;
    case 2:
        connectivityList.push_back(zz * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + 1 + start);
        break;
    case 3:
        connectivityList.push_back(zz * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + 1 + start);
        break;
    case 4:
        connectivityList.push_back(zz * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + start);
        break;
    case 5:
        connectivityList.push_back((zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + start);
        break;
    case 6:
        connectivityList.push_back((zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + 1 + start);
        break;
    case 7:
        connectivityList.push_back((zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + 1 + start);
        break;
    case 8:
        connectivityList.push_back((zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + start);
        break;
    }







}

void PartitionReader::addToBlockConnectivityList(vector<vistle::Index> &block, int index, int start, int xx, int yy, int zz) {
    switch (index) {
    case 1:
        block[index - 1] = zz * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + start;
        break;
    case 2:
        block[index - 1] = zz * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + 1 + start;
        break;
    case 3:
        block[index - 1] = zz * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + 1 + start;
        break;
    case 4:
        block[index - 1] = zz * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + start;
        break;
    case 5:
        block[index - 1] = (zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + start;
        break;
    case 6:
        block[index - 1] = (zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + yy * (blockDimensions[0]) + xx + 1 + start;
        break;
    case 7:
        block[index - 1] = (zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + 1 + start;
        break;
    case 8:
        block[index - 1] = (zz + 1) * (blockDimensions[1]) * (blockDimensions[0]) + (yy + 1) * (blockDimensions[0]) + xx + start;
        break;
    }







}

vector<int> PartitionReader::getIndicesBetweenCorners(std::vector<int> corners)     {
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
            for (size_t i = cornerIndexToBlockIndex(corners[0]); i <= cornerIndexToBlockIndex(corners[3]); i++) {
                indices.push_back(i);
            }
        }
        else if (corners[3] - corners[0] == 5) { //xz plane
            int start = cornerIndexToBlockIndex(corners[0]);
            for (size_t i = 0; i < blockDimensions[2]; i++) {
                for (size_t j = 0; j < blockDimensions[1]; j++) {
                    indices.push_back(start + j);
                }
                start += blockDimensions[0] * blockDimensions[1];
            }
        }
        else if (corners[3] - corners[0] == 6) {//yz plane
            for (size_t i = cornerIndexToBlockIndex(corners[0]); i <= cornerIndexToBlockIndex(corners[3]); i += blockDimensions[0]) {
                indices.push_back(i);
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

vector<array<int, 2>> PartitionReader::getAllEdgesInCornerIndices()     {
    vector < array<int, 2>> edges;
    edges.push_back({ 1,2 });
    edges.push_back({ 1,3 });
    edges.push_back({ 2,4 });
    edges.push_back({ 3,4 });
    if (dim == 3) {
        edges.push_back({ 1,5 });
        edges.push_back({ 2,6 });
        edges.push_back({ 3,7 });
        edges.push_back({ 4,6 });
        edges.push_back({ 5,6 });
        edges.push_back({ 5,7 });
        edges.push_back({ 6,8 });
        edges.push_back({ 7,8 });
    }
    return edges;
}

vector<array<int, 4>> PartitionReader::getAllPlanesInCornerIndices() {
    vector < array<int, 4>> planes;
    planes.push_back({ 1, 2, 3, 4 });
    planes.push_back({ 1, 2, 5, 6 });
    planes.push_back({ 1, 3, 5, 7 });
    planes.push_back({ 2, 4, 6, 8 });
    planes.push_back({ 3, 4, 7, 8 });
    planes.push_back({ 5, 6, 7, 8 });

    return planes;
}

void PartitionReader::fillConnectivityList(const map<int, int> &localToGloabl) {
    int start = connectivityList.size() / numCorners / hexesPerBlock * blockSize;
    int numDoublePoints = 0;
    int numLocalEntries = 0;
    int dimZ = dim == 2 ? 1 : blockDimensions[2] - 1;

    for (int xx = 0; xx < blockDimensions[0] - 1; xx++) {
        for (int yy = 0; yy < blockDimensions[1] - 1; yy++) {
            for (int zz = 0; zz < dimZ; zz++) {
                for (size_t index = 1; index <= numCorners; index++) {
                    bool added = false;
                    if (localToGloabl.size() > 1) {
                        int curIndex = connectivityList[numLocalEntries];
                        auto it = localToGloabl.find(curIndex);
                        if (it != localToGloabl.end()) {
                            connectivityList.push_back(it->second);
                            ++numDoublePoints;
                            added = true;
                        }
                    }
                    if (!added) {
                        addToConnectivityList(index, start - numDoublePoints, xx, yy, zz);
                    }
                    ++numLocalEntries;
                }
            }
        }
    }
}

void PartitionReader::fillConnectivityList2(const map<int, int>& localToGloabl, int startIndexInMesh) {

    set<int>doublePoints;
    int currBlock = 0;
    int dimZ = dim == 2 ? 1 : blockDimensions[2] - 1;
    vector<int> writeOrder = { 1, 2, 4, 3, 5, 6, 8, 7 };
    for (int xx = 0; xx < blockDimensions[0] - 1; xx++) {
        for (int yy = 0; yy < blockDimensions[1] - 1; yy++) {
            for (int zz = 0; zz < dimZ; zz++) {
                vector<vistle::Index> block(numCorners);
                for (size_t i = 0; i < numCorners; ++i) {
                    int index = writeOrder[i];
                    bool added = false;
                    if (localToGloabl.size() > 1) {
                        int curIndex = connectivityList[currBlock*numCorners + index - 1];
                        auto it = localToGloabl.find(curIndex);
                        if (it != localToGloabl.end()) {
                            block[index - 1] = it->second;
                            doublePoints.insert(it->second);
                            added = true;
                        }
                    }
                    if (!added) {
                        addToBlockConnectivityList(block, index, startIndexInMesh - doublePoints.size(), xx, yy, zz);
                    }

                }
                connectivityList.insert(connectivityList.end(), block.begin(), block.end());
                ++currBlock;
            }
        }
    }
}

void PartitionReader::fillConnectivityList3(const map<int, int>& localToGloabl, int startIndexInMesh) {

    vector<vistle::Index> connList(numCorners * hexesPerBlock);
    int numDoubles = 0;

    for (size_t index = 0; index < blockSize; index++) {
        auto conListEntries = blockIndexToConnectivityIndex.find(index)->second;
        bool added = false;
        if (localToGloabl.size() > 1) {
            auto it = localToGloabl.find(index);
            if (it != localToGloabl.end()) {
                for (auto entry : conListEntries) {
                    connList[entry] = it->second;
                }
                ++numDoubles;
                added = true;
            }
        }
        if (!added) {
            for (auto entry : conListEntries) {
                connList[entry] = startIndexInMesh + index - numDoubles;
            }
        }
    }
    connectivityList.insert(connectivityList.end(), connList.begin(), connList.end());
}


void PartitionReader::constructBlockIndexToConnectivityIndex()     {
    blockIndexToConnectivityIndex.clear();
    for (size_t j = 0; j < blockSize; j++) {
        vector<int> connectivityIndices;
        auto index = baseConnList.begin(), end = baseConnList.end();
        while (index != end) {
            index = std::find(index, end, j);
            if (index != end) {
                int i = index - baseConnList.begin();
                connectivityIndices.push_back(i);
                ++index;
            }
        }
        blockIndexToConnectivityIndex[j] = connectivityIndices;
    }
}

void PartitionReader::addNewCorners(map<int, int> &allCorners, int localBlock)     {
    auto corners = mapFileData[myBlocksToRead[localBlock]];
    for (size_t i = 1; i <= numCorners; i++) {
        allCorners[corners[i]] = connectivityList[blockIndexToConnectivityIndex.find(cornerIndexToBlockIndex(i))->second[0] + localBlock * numCorners * hexesPerBlock];
    }
}

void PartitionReader::addNewEdges(map<array<int, 2>, vector<int>> &allEdges, int localBlock) {
    
    auto corners = mapFileData[myBlocksToRead[localBlock]];
    vector<array<int, 2>> edges = getAllEdgesInCornerIndices();
    for (auto edge : edges) {
        vector<int> blockIndices = getIndicesBetweenCorners({ edge[0], edge[1] });
        vector<int> gridIndices;
        gridIndices.reserve(blockIndices.size());
        for (int blockIndex : blockIndices)             {
            gridIndices.push_back(connectivityList[blockIndexToConnectivityIndex.find(blockIndex)->second[0] + localBlock * numCorners * hexesPerBlock]);
        }
        
        
        array<int, 2> e = { corners[edge[0]], corners[edge[1]] };
        sort(e.begin(), e.end());
        allEdges[e] = gridIndices;
    }
}

void PartitionReader::addNewPlanes(map<array<int, 4>, vector<int>>& allEdges, int localBlock) {

    auto corners = mapFileData[myBlocksToRead[localBlock]];
    vector<array<int, 4>> planes = getAllPlanesInCornerIndices();
    for (auto plane : planes) {
        vector<int> blockIndices = getIndicesBetweenCorners({ plane[0], plane[1], plane[2], plane[3] });
        vector<int> gridIndices;
        gridIndices.reserve(blockIndices.size());
        for (int blockIndex : blockIndices) {
            gridIndices.push_back(connectivityList[blockIndexToConnectivityIndex.find(blockIndex)->second[0] + localBlock * numCorners * hexesPerBlock]);
        }


        array<int, 4> e = { corners[plane[0]], corners[plane[1]], corners[plane[2]], corners[plane[3]] };
        sort(e.begin(), e.end());
        allEdges[e] = gridIndices;
    }
}

}

