#include "PartitionReader.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

namespace nek5000 {
//public methods
PartitionReader::PartitionReader(ReaderBase &base)
    :ReaderBase((base))
{

}


bool PartitionReader::fillConnectivityList(vistle::Index *connectivities)
{
    using vistle::Index;
        Index* nl = connectivities;
        for (Index elements_so_far = 0; elements_so_far < myBlocksToRead; ++elements_so_far) {
            long pt_start = blockSize * elements_so_far;
            for (Index ii = 0; ii < iBlockSize[0] - 1; ii++) {
                for (Index jj = 0; jj < iBlockSize[1] - 1; jj++) {
                    if (iDim == 2) {
                        *nl++ = jj * (iBlockSize[0]) + ii + pt_start;
                        *nl++ = jj * (iBlockSize[0]) + ii + 1 + pt_start;
                        *nl++ = (jj + 1) * (iBlockSize[0]) + ii + 1 + pt_start;
                        *nl++ = (jj + 1) * (iBlockSize[0]) + ii + pt_start;
                    } else {
                        for (Index kk = 0; kk < iBlockSize[2] - 1; kk++) {
                            *nl++ = kk * (iBlockSize[1]) * (iBlockSize[0]) + jj * (iBlockSize[0]) + ii + pt_start;
                            *nl++ = kk * (iBlockSize[1]) * (iBlockSize[0]) + jj * (iBlockSize[0]) + ii + 1 + pt_start;
                            *nl++ = kk * (iBlockSize[1]) * (iBlockSize[0]) + (jj + 1) * (iBlockSize[0]) + ii + 1 + pt_start;
                            *nl++ = kk * (iBlockSize[1]) * (iBlockSize[0]) + (jj + 1) * (iBlockSize[0]) + ii + pt_start;
                            *nl++ = (kk + 1) * (iBlockSize[1]) * (iBlockSize[0]) + jj * (iBlockSize[0]) + ii + pt_start;
                            *nl++ = (kk + 1) * (iBlockSize[1]) * (iBlockSize[0]) + jj * (iBlockSize[0]) + ii + 1 + pt_start;
                            *nl++ = (kk + 1) * (iBlockSize[1]) * (iBlockSize[0]) + (jj + 1) * (iBlockSize[0]) + ii + 1 + pt_start;
                            *nl++ = (kk + 1) * (iBlockSize[1]) * (iBlockSize[0]) + (jj + 1) * (iBlockSize[0]) + ii + pt_start;
                        }
                    }
                }
            }
        }
    return true;
}

bool PartitionReader::fillMesh(float *x, float *y, float *z)
{
    for (size_t b = 0; b < myBlocksToRead; b++) {

        if (!ReadMesh(timestepToUseForMesh, myFirstBlockToRead + b, x + b * blockSize, y + b * blockSize, z + b * blockSize)) {
            return false;
        }
    }
}

bool PartitionReader::fillVelocity(int timestep, float *x, float *y, float *z)
{
    if (bHasVelocity) {
        for (int b = 0; b < myBlocksToRead; b++) {
            if (!ReadVelocity(timestep, myFirstBlockToRead + b, x + b * blockSize, y + b * blockSize, z + b * blockSize)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool PartitionReader::fillScalarData(std::string varName, int timestep, float *data)
{
    for (int b = 0; b < myBlocksToRead; b++) {
        if (!ReadVar(varName, timestep, myFirstBlockToRead + b, data + b * blockSize)) {
            return false;
        }
    }
}



//getter
size_t PartitionReader::getBlocksToRead() const
{
    return myBlocksToRead;
}
size_t PartitionReader::getFirstBlockToRead() const
{
    return myFirstBlockToRead;
}
size_t PartitionReader::getHexes()const
{
    int hexes_per_element = (iBlockSize[0] - 1) * (iBlockSize[1] - 1);
    if (iDim == 3)
        hexes_per_element *= (iBlockSize[2] - 1);
    return hexes_per_element * myBlocksToRead;
}
size_t PartitionReader::getNumConn() const
{
        int numConn = (iDim == 3 ? 8  : 4 );
        return numConn * getHexes();
}

size_t PartitionReader::getGridSize() const
{
    return blockSize * myBlocksToRead;
}




//setter
bool PartitionReader::setPartition(int partition)
{
    if(partition > numPartitions)
        return false;
    myPartition = partition;
    int blocks_per_partition = numBlocksToRead / numPartitions;
    int one_extra_until = numBlocksToRead % numPartitions;
    myBlocksToRead = blocks_per_partition + (myPartition < one_extra_until ? 1 : 0);
    myFirstBlockToRead = blocks_per_partition * myPartition + (myPartition < one_extra_until ? myPartition : one_extra_until);
    if(!ReadBlockLocations())
        return false;
    return true;
}

//private methods
bool PartitionReader::AssembleBlockMap(std::vector<int> blockList, bool fromMap, std::map<int, std::pair<int, int>> blockMap)
{
    myBlockIDs.reserve(myBlocksToRead);
    for(int i=0; i < myBlocksToRead; ++i)
    {
      myBlockIDs.emplace_back(blockList[myFirstBlockToRead + i]);
    }
    if(fromMap)
    {
        std::sort(myBlockIDs.begin(), myBlockIDs.end());
    }
    myBlockPositions.reserve(myBlocksToRead);
    for(int i=0; i < myBlocksToRead; ++i)
    {
      myBlockPositions[i] = blockMap.find(myBlockIDs[i])->second.second;
    }

    // TEMP: checking for duplicates within myBlockPositions
    if(fromMap)
    {
      for(int i=0; i<this->myBlocksToRead-1; i++)
      {
        for(int j=i+1; j<this->myBlocksToRead; j++)
        {
          if(this->myBlockPositions[i] == this->myBlockPositions[j])
          {
            std::cerr<<"********my_partition: "<< myPartition<< " : Hey (this->myBlockPositions["<<i<<"] and ["<<j<<"] both == "<< this->myBlockPositions[j]<< std::endl;
          }
        }
      }
    }
}


bool PartitionReader::ReadMesh(int timestep, int block, float *x, float *y, float* z) {
    int fileID = getFileID(block);
    if(!CheckOpenFile(curOpenMeshFile, timestep, fileID))
        return false;

    if (isParalellFormat)
        block = blockMap[block].second;

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, string());
    int nFloatsInDomain = dp.domSizeInFloats;
    long iRealHeaderSize = iHeaderSize + (isParalellFormat ? vBlocksPerFile[fileID] * sizeof(int) : 0);

    if (isBinary) {
        //In the parallel format, the whole mesh comes before all the vars.
        if (isParalellFormat)
            nFloatsInDomain = iDim * iBlockSize[0] * iBlockSize[1] * iBlockSize[2];

        if (iPrecision == 4) {
             fseek(curOpenMeshFile->file(), iRealHeaderSize + (long)nFloatsInDomain * sizeof(float) * block, SEEK_SET);
            size_t res = fread(x, sizeof(float), blockSize, curOpenMeshFile->file()); (void)res;
            res = fread(y, sizeof(float), blockSize, curOpenMeshFile->file()); (void)res;
            if (iDim == 3) {
                size_t res = fread(z, sizeof(float), blockSize, curOpenMeshFile->file()); (void)res;
            }
            else {
                memset(z, 0, blockSize * sizeof(float));
            }
            if (bSwapEndian)                 {
                ByteSwapArray(x, blockSize);
                ByteSwapArray(y, blockSize);
                if (iDim == 3) {
                    ByteSwapArray(z, blockSize);
                }
            }

        } else {
            double* tmppts = new double[blockSize * iDim];
            fseek(curOpenMeshFile->file(), iRealHeaderSize +
                (long)nFloatsInDomain * sizeof(double) * block,
                SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), blockSize * iDim, curOpenMeshFile->file()); (void)res;
            if (bSwapEndian)
                ByteSwapArray(tmppts, blockSize * iDim);
            for (int i = 0; i < blockSize; i++) {
                x[i] = static_cast<int>(tmppts[i]);
            }
            for (int i = 0; i < blockSize; i++) {
                y[i] = static_cast<int>(tmppts[i + blockSize]);
            }
            if (iDim == 3) {
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
            if (iDim == 3) {
                int res = fscanf(curOpenMeshFile->file(), " %f %f %f", &x[ii], &y[ii], &z[ii]); (void)res;
            } else {
                int res = fscanf(curOpenMeshFile->file(), " %f %f", &x[ii], &y[ii]); (void)res;
                memset(z, 0, blockSize * sizeof(float));
            }
        }
    }
    return true;
}

bool PartitionReader::ReadVelocity(int timestep, int block, float* x, float* y, float* z) {
    int fileID = getFileID(block);
    if(!CheckOpenFile(curOpenVarFile, timestep, fileID))
        return false;

    DomainParams dp = GetDomainSizeAndVarOffset(timestep, "velocity");
    if (isParalellFormat)
        block = blockMap[block].second;

    long iRealHeaderSize = iHeaderSize + (isParalellFormat ? vBlocksPerFile[fileID] * sizeof(int) : 0);

    if (isBinary) {
        long filepos;
        if (!isParalellFormat)
            filepos = (long)iRealHeaderSize + (long)(dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else
            //This assumes [block 0: 216u 216v 216w][block 1: 216u 216v 216w]...[block n: 216u 216v 216w]
            filepos = (long)iRealHeaderSize +
            (long)vBlocksPerFile[fileID] * dp.varOffsetBinary * iPrecision + //the header and mesh if one exists
            (long)block * blockSize * iDim * iPrecision;
        if (iPrecision == 4) {
            fseek(curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(x, sizeof(float), blockSize, curOpenVarFile->file()); (void)res;
            res = fread(y, sizeof(float), blockSize, curOpenVarFile->file()); (void)res;
            if (iDim == 3) {
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
                if (iDim == 3) {
                    ByteSwapArray(z, blockSize);
                }
            }
        } else {
            double* tmppts = new double[blockSize * iDim];
            fseek(curOpenVarFile->file(), filepos, SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), blockSize * iDim, curOpenVarFile->file()); (void)res;

            if (bSwapEndian)
                ByteSwapArray(tmppts, blockSize * iDim);

            for (int ii = 0; ii < blockSize; ii++) {
                x[ii] = (double)tmppts[ii];
                y[ii] = (double)tmppts[ii + blockSize];
                if (iDim == 3) {
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
            if (iDim == 3) {
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
                    (long)dp.timestepHasMesh * vBlocksPerFile[fileID] * blockSize * iDim * iPrecision + //mesh
                    (long)block * blockSize * iDim * iPrecision +                  //start of block
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
    bool fromMap = true;
    if(!ParseGridMap())
    {
        all_element_list = tmpBlocks;
        fromMap = false;
    }
    myBlockIDs.clear();
    myBlockIDs.reserve(myBlocksToRead);
    for(int i=0; i < myBlocksToRead; ++i)
    {
      myBlockIDs.emplace_back(all_element_list[myFirstBlockToRead + i]);
    }
    if(fromMap)
    {
        std::sort(myBlockIDs.begin(), myBlockIDs.end());
    }
    myBlockPositions.clear();
    myBlockPositions.reserve(myBlocksToRead);
    for(int i=0; i < myBlocksToRead; ++i)
    {
      myBlockPositions.emplace_back(blockMap.find(myBlockIDs[i])->second.second);
    }

    // TEMP: checking for duplicates within myBlockPositions
    if(fromMap)
    {
      for(int i=0; i<this->myBlocksToRead-1; i++)
      {
        for(int j=i+1; j<this->myBlocksToRead; j++)
        {
          if(this->myBlockPositions[i] == this->myBlockPositions[j])
          {
            sendError("nek5000: my_partition: " + to_string(myPartition) + " : Hey (this->myBlockPositions[" + to_string(i) + "] and [" + to_string(j) + "] both == " + to_string(this->myBlockPositions[j]));
            return false;
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
        nFloatsPerSample += iDim;
    if (bHasVelocity)
        nFloatsPerSample += iDim;
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
                iNumPrecedingFloats += iDim;
        } else if (var == "y_velocity") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;

            iNumPrecedingFloats += 1;
        } else if (var == "z_velocity") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;

            iNumPrecedingFloats += 2;
        } else if (var == "pressure") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;
            if (bHasVelocity)
                iNumPrecedingFloats += iDim;
        } else if (var == "temperature") {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;
            if (bHasVelocity)
                iNumPrecedingFloats += iDim;
            if (bHasPressure)
                iNumPrecedingFloats += 1;
        } else if (var[0] == 's') {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;
            if (bHasVelocity)
                iNumPrecedingFloats += iDim;
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

if(file)
    cerr << "checking open file, open file is " << file->name() << ", looking for " << filename << endl;
else
    cerr << "checking open file, open file is closed" << endl;

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




}

