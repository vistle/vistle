/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

 /**************************************************************************\
  **                                                           (C)1995 RUS  **
  **                                                                        **
  ** Description: Read module for Nek5000 data                              **
  **                                                                        **
  **                                                                        **
  **                                                                        **
  **                                                                        **
  **                                                                        **
  ** Author:                                                                **
  **                                                                        **
  **                             Dennis Grieger                             **
  **                                 HLRS                                   **
  **                            Nobelstraße 19                              **
  **                            70550 Stuttgart                             **
  **                                                                        **
  ** Date:  08.07.19  V1.0                                                  **
 \**************************************************************************/


#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#endif

#include "ReadNek5000.h"

#include <util/enum.h>
#include <util/byteswap.h>

#include "ReadNek5000.h"

#include <stdio.h>

#include <core/vec.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>


using namespace vistle;
using namespace std;
namespace fs = boost::filesystem;

#ifndef STREQUAL
#if defined(_WIN32) 
#  define STREQUAL(a,b)              stricmp(a,b)
#else
#  define STREQUAL(a,b)              strcasecmp(a,b)
#endif
#endif
using std::string;
bool ReadNek::prepareRead() {
    UpdateCyclesAndTimes();   //This call also finds which timesteps have a mesh.
    ReadBlockLocations();
    numReads = 0;
    return true;
}

bool ReadNek::read(Token& token, int timestep, int partition) {
    ++numReads;

    int blocksToRead = iNumBlocks / p_numPartitions->getValue();
    if (partition == p_numPartitions->getValue() - 1) {
        blocksToRead += iNumBlocks % p_numPartitions->getValue(); //the last one has to read the rest
    }
    int blocksRead = partition * std::floor(iNumBlocks / p_numPartitions->getValue());
    if (timestep == -1) {//there is only one grid for all timesteps, so we read it in advance
        
        int hexes_per_element = (iBlockSize[0] - 1) * (iBlockSize[1] - 1);
        if (iDim == 3)
            hexes_per_element *= (iBlockSize[2] - 1);
        int total_hexes = hexes_per_element * blocksToRead;
        int numConn = (iDim == 3 ? 8 * total_hexes : 4 * total_hexes);

        UnstructuredGrid::ptr grid = UnstructuredGrid::ptr(new UnstructuredGrid(total_hexes, numConn, blocksToRead * iTotalBlockSize));


        if (iDim == 2) {
            std::fill_n(grid->tl().data(), total_hexes, (unsigned)UnstructuredGrid::QUAD);
        } else {
            std::fill_n(grid->tl().data(), total_hexes, (unsigned)UnstructuredGrid::HEXAHEDRON);
        }
        makeConectivityList(grid->cl().data(), blocksToRead);
        for (size_t i = 0; i < total_hexes + 1; i++) {
            if (iDim == 2) {
                grid->el().data()[i] = 4 * i;
            } else {
                grid->el().data()[i] = 8 * i;
            }
        }
        mGrids[partition] = grid;
        grid->setBlock(partition);
        grid->setTimestep(-1);
        grid->applyDimensionHint(grid);
        auto x = grid->x().data();
        auto y = grid->y().data();
        auto z = grid->z().data();
        for (size_t b = 0; b < blocksToRead; b++) {

            if (!ReadMesh(timestepToUseForMesh, blocksRead + b, x + b * iTotalBlockSize, y + b * iTotalBlockSize, z + b * iTotalBlockSize)) {
                return false;
            }
        }
        token.addObject(p_grid, grid);
    }
    else if (!p_only_geometry->getValue()){
        {//velocities
            auto grid = mGrids.find(partition);
            if (grid == mGrids.end()) {
                sendError(".nek5000 did not find a matching grid for block %d", partition);
                return false;
            }
            Vec<float, 3>::ptr velocity(new Vec<Scalar, 3>(iTotalBlockSize * blocksToRead));
            velocity->setGrid(grid->second);
            velocity->setTimestep(timestep);
            velocity->setBlock(partition);
            velocity->addAttribute("_species", "velocity");
            velocity->applyDimensionHint(velocity);
            auto x = velocity->x().data();
            auto y = velocity->y().data();
            auto z = velocity->z().data();
            if (bHasVelocity) {
                for (size_t b = 0; b < blocksToRead; b++) {
                    if (!ReadVelocity(timestep, blocksRead + b, x + b * iTotalBlockSize, y + b * iTotalBlockSize, z + b * iTotalBlockSize)) {
                        return false;
                    }
                }
            }
            grid->second->updateInternals();
            token.addObject(p_velocity, velocity);
        }
        if (bHasPressure) {
            if (!ReadScalarData(token, p_pressure, "pressure", timestep, partition)) {
                return false;
            }
        }
        if (bHasTemperature) {
            if (!ReadScalarData(token, p_temperature, "temperature", timestep, partition)) {
                return false;
            }
        }
        for (size_t i = 0; i < iNumSFields; i++) {
            if (!ReadScalarData(token, pv_misc[i], "s" + std::to_string(i + 1), timestep, partition)) {
                return false;
            }
        }

    }
    return true;
}

bool ReadNek::examine(const vistle::Parameter* param) {
   
    iNumBlocks = p_numBlocks->getValue();
    if (iNumBlocks <= 0 ||iNumBlocks > iTotalNumBlocks) {
        iNumBlocks = iTotalNumBlocks;
    }
    int oldNumSFields = iNumSFields;
    if (fs::exists(p_data_path->getValue())) {
        if (!parseMetaDataFile() || !ParseNekFileHeader()) {
            return false;
        }
        setTimesteps(iNumTimesteps);
        setPartitions(p_numPartitions->getValue());
    }
    for (size_t i = oldNumSFields; i < iNumSFields; i++) {
        pv_misc.push_back(createOutputPort("scalarFiled" + std::to_string(i + 1), "scalar data " + std::to_string(i + 1)));
    }
    for (size_t i = oldNumSFields; i > iNumSFields; --i) {
        destroyPort(pv_misc[i]);
    }
    return true;

}

bool ReadNek::finishRead() {
    std::cerr << "_________________________________________________________" << std::endl;
    std::cerr << "read was called "<< numReads << " times" << std::endl;
    std::cerr << "_________________________________________________________" << std::endl;
    return true;
}



bool ReadNek::parseMetaDataFile() {
    string tag;
    char buf[2048];
    ifstream  f(p_data_path->getValue());
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

        if (STREQUAL("endian:", tag.c_str()) == 0) {
            //This tag is deprecated.  There's a float written into each binary file
            //from which endianness can be determined.
            string  dummy_endianness;
            f >> dummy_endianness;
        } else if (STREQUAL("filetemplate:", tag.c_str()) == 0) {
            f >> fileTemplate;
        } else if (STREQUAL("firsttimestep:", tag.c_str()) == 0) {
            f >> iFirstTimestep;
        } else if (STREQUAL("numtimesteps:", tag.c_str()) == 0) {
            f >> iNumTimesteps;
        } else if (STREQUAL("meshcoords:", tag.c_str()) == 0) {
            //This tag is now deprecated.  The same info can be discovered by
            //this reader while it scans all the headers for time and cycle info.

            int nStepsWithCoords;
            f >> nStepsWithCoords;

            for (ii = 0; ii < nStepsWithCoords; ii++) {
                int step;
                f >> step;
            }
        } else if (STREQUAL("type:", tag.c_str()) == 0) {
            //This tag can be deprecated, because the type can be determined
            //from the header 
            string t;
            f >> t;
            if (STREQUAL("binary", t.c_str()) == 0) {
                bBinary = true;
            } else if (STREQUAL("binary6", t.c_str()) == 0) {
                bBinary = true;
                bParFormat = true;
            } else if (STREQUAL("ascii", t.c_str()) == 0) {
                bBinary = false;
            } else {
                sendError(".nek5000: Value following \"type\" must be \"ascii\" or \"binary\" or \"binary6\"");
                if (f.is_open())
                    f.close();
                return false;
            }
        } else if (STREQUAL("numoutputdirs:", tag.c_str()) == 0) {
            //This tag can be deprecated, because the number of output dirs 
            //can be determined from the header, and the parallel nature of the
            //file can be inferred from the number of printf tokens in the template.
            //This reader scans the headers for the number of fld files
            f >> iNumOutputDirs;
            if (iNumOutputDirs > 1)
                bParFormat = true;
        } else if (STREQUAL("timeperiods:", tag.c_str()) == 0) {
            f >> numberOfTimePeriods;
        } else if (STREQUAL("gapBetweenTimePeriods:", tag.c_str()) == 0) {
            f >> gapBetweenTimePeriods;
        } else if (STREQUAL("NEK3D", tag.c_str()) != 0) {
            //This is an obsolete tag, ignore it.
        } else if (STREQUAL("version:", tag.c_str()) != 0) {
            //This is an obsolete tag, ignore it, skipping the version number
            //as well.
            string version;
            f >> version;
        } else {
            sendError(".nek5000: Error parsing file.  Unknown tag %s", tag.c_str());
            if (f.is_open())
                f.close();
            return false;
        }
    }

    if (numberOfTimePeriods < 1) {
        sendError(".nek5000: The number of time periods must be 1 or more.");
    }
    if (numberOfTimePeriods > 1 && gapBetweenTimePeriods <= 0.0) {
        sendError(".nek5000: The gap between time periods must be non-zero.");
        if (f.is_open())
            f.close();
        return false;
    }

    //Do a little consistency checking before moving on
    if (fileTemplate == "") {
        sendError(".nek5000: A tag called filetemplate: must be specified");
        if (f.is_open())
            f.close();
        return false;
    }
    f.close();

    // make the file template, which is normally relative to the file being opened, 
    // into an absolute path
    if (fileTemplate[0] != '/') {
        for (ii = p_data_path->getValue().size(); ii >= 0; ii--) {
            if (p_data_path->getValue()[ii] == '/' || p_data_path->getValue()[ii] == '\\') {
                fileTemplate.insert(0, p_data_path->getValue().c_str(), ii + 1);
                break;
            }
        }
        if (ii == -1) {
#ifdef _WIN32
            _getcwd(buf, 512);
#else
            char* res = getcwd(buf, 512); (void)res;
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
    if (f.is_open())
        f.close();
    return true;
}

bool ReadNek::ParseNekFileHeader() {
    string buf2, tag;

    //Now read the header out of one the files to get block and variable info
    string blockfilename = GetFileName(0, 0);
    ifstream  f(blockfilename, ifstream::binary);

    if (!f.is_open()) {
        sendError("Could not open file %s, which should exist according to header file %s.", blockfilename.c_str(), p_data_path->getValue().c_str());
        return false;
    }

    // Determine the type (ascii or binary)
    // Parallel type is determined by the number of tokens in the file template
    // and is always binary
    if (!bParFormat) {
        float test;
        f.seekg(80, ios::beg);

        f.read((char*)(&test), 4);
        if (test > 6.5 && test < 6.6)
            bBinary = true;
        else {
            ByteSwapArray(&test, 1);
            if (test > 6.5 && test < 6.6)
                bBinary = true;
        }
        f.seekg(0, ios::beg);
    }

    //iHeaderSize no longer includes the size of the block index metadata, for the 
    //parallel format, since this now can vary per file.
    if (bBinary && bParFormat)
        iHeaderSize = 136;
    else if (bBinary && !bParFormat)
        iHeaderSize = 84;
    else
        iHeaderSize = 80;


    if (!bParFormat) {
        f >> iTotalNumBlocks;
        f >> iBlockSize[0];
        f >> iBlockSize[1];
        f >> iBlockSize[2];

        f >> buf2;   //skip
        f >> buf2;   //skip

        ParseFieldTags(f);
    } else {
        //Here's are some examples of what I'm parsing:
        //#std 4  6  6  6   120  240  0.1500E+01  300  1  2XUPT
        //#std 4  6  6  6   120  240  0.1500E+01  300  1  2 U T123
        //This example means:  #std is for versioning, 4 bytes per sample,  
        //  6x6x6 blocks, 120 of 240 blocks are in this file, time=1.5, 
        //  cycle=300, this output dir=1, num output dirs=2, XUPT123 are 
        //  tags that this file has a mesh, velocity, pressure, temperature, 
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
            if (f.is_open()) {
                f.close();
            }
            return false;
        }
        f >> iPrecision;
        f >> iBlockSize[0];
        f >> iBlockSize[1];
        f >> iBlockSize[2];
        f >> buf2;  //blocks per file
        f >> iTotalNumBlocks;

        //This bypasses some tricky and unnecessary parsing of data
        //I already have.  
        //6.13.08  No longer works...
        //f.seekg(77, std::ios_base::beg);
        f >> buf2;  //time
        f >> buf2;  //cycle
        f >> buf2;  //directory num of this file

        //I do this to skip the num directories token, because it may abut 
        //the field tags without a whitespace separator.
        while (f.peek() == ' ')
            f.get();

        //The number of output dirs comes next, but may abut the field tags
        iNumOutputDirs = 0;
        while (f.peek() >= '0' && f.peek() <= '9') {
            iNumOutputDirs *= 10;
            iNumOutputDirs += (f.get() - '0');
        }

        ParseFieldTags(f);

    }

    if (iBlockSize[2] == 1)
        iDim = 2;
    iTotalBlockSize = iBlockSize[0] * iBlockSize[1] * iBlockSize[2];
    if (bBinary) {
        // Determine endianness and whether we need to swap bytes.
        // If this machine's endian matches the file's, the read will 
        // put 6.54321 into this float.

        float test;
        if (!bParFormat) {
            f.seekg(80, std::ios_base::beg);
            f.read((char*)(&test), 4);
        } else {
            f.seekg(132, std::ios_base::beg);
            f.read((char*)(&test), 4);
        }
        if (test > 6.5 && test < 6.6)
            bSwapEndian = false;
        else {
            ByteSwapArray(&test, 1);
            if (test > 6.5 && test < 6.6)
                bSwapEndian = true;
            else {
                sendError("Nek: Error reading file, while trying to determine endianness.");
                if (f.is_open()) {
                    f.close();
                }
                return false;
            }
        }
    }
    // Now that we know iNumBlocks, we can set up what the "typical elements"
    // are for each processor.
    //int rank = PAR_Rank();
    //int nprocs = PAR_Size();
    //int elements_per_proc = iNumBlocks / nprocs;
    //int one_extra_until = iNumBlocks % nprocs;
    //int my_num_elements = elements_per_proc + (rank < one_extra_until ? 1 : 0);
    //cachableElementMin = elements_per_proc * rank + (rank < one_extra_until ? rank : one_extra_until);
    //cachableElementMax = cachableElementMin + my_num_elements;
    return true;
}

void ReadNek::ParseFieldTags(ifstream& f) {
    iNumSFields = 1;
    int numSpacesInARow = 0;
    bool foundCoordinates = false;
    while (f.tellg() < iHeaderSize) {
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
            bHasVelocity = true;
        else if (c == 'P')
            bHasPressure = true;
        else if (c == 'T')
            bHasTemperature = true;
        else if (c >= '1' && c <= '9') {
            // If we have S##, then it will be caught in the 'S'
            // logic below.  So this means that we have the legacy
            // representation and there is between 1 and 9 scalars.
            iNumSFields = c - '0';
        } else if (c == 'S') {
            while (f.peek() == ' ')
                f.get();
            char digit1 = f.get();
            while (f.peek() == ' ')
                f.get();
            char digit2 = f.get();

            if (digit1 >= '0' && digit1 <= '9' &&
                digit2 >= '0' && digit2 <= '9')
                iNumSFields = (digit1 - '0') * 10 + (digit2 - '0');
            else
                iNumSFields = 1;
        } else
            break;
    }
    if (!foundCoordinates) {
        sendError("Nek: The first time step in a Nek file must contain a mesh");

    }
}






bool ReadNek::ReadMesh(int timestep, int block, float *x, float *y, float* z) {
    if (fdMesh == NULL || (bParFormat && aBlockLocs[block * 2] != iCurrMeshProc)) {

        iCurrMeshProc = 0;
        if (bParFormat)
            iCurrMeshProc = aBlockLocs[block * 2];

        string meshfilename = GetFileName(timestep, iCurrMeshProc);

        if (curOpenMeshFile != meshfilename) {
            if (fdMesh)
                fclose(fdMesh);

            fdMesh = fopen(meshfilename.c_str(), "rb");
            curOpenMeshFile = meshfilename;
            if (!fdMesh)                 {
                sendError("nek5000: can't open file: " + meshfilename);
                return false;
            }
        }
        if (!bBinary)
            FindAsciiDataStart(fdMesh, iAsciiMeshFileStart, iAsciiMeshFileLineLen);
    }

    if (bParFormat)
        block = aBlockLocs[block * 2 + 1];


    DomainParams dp = GetDomainSizeAndVarOffset(timestep, NULL);
    int nFloatsInDomain = dp.domSizeInFloats;
    long iRealHeaderSize = iHeaderSize + (bParFormat ? vBlocksPerFile[iCurrMeshProc] * sizeof(int) : 0);

    if (bBinary) {
        //In the parallel format, the whole mesh comes before all the vars.
        if (bParFormat)
            nFloatsInDomain = iDim * iBlockSize[0] * iBlockSize[1] * iBlockSize[2];

        if (iPrecision == 4) {
             fseek(fdMesh, iRealHeaderSize + (long)nFloatsInDomain * sizeof(float) * block, SEEK_SET);
            size_t res = fread(x, sizeof(float), iTotalBlockSize, fdMesh); (void)res;
            res = fread(y, sizeof(float), iTotalBlockSize, fdMesh); (void)res;
            if (iDim == 3) {
                size_t res = fread(z, sizeof(float), iTotalBlockSize, fdMesh); (void)res;
            }
            else {
                memset(z, 0, iTotalBlockSize * sizeof(float));
            }
            if (bSwapEndian)                 {
                ByteSwapArray(x, iTotalBlockSize);
                ByteSwapArray(y, iTotalBlockSize);
                if (iDim == 3) {
                    ByteSwapArray(z, iTotalBlockSize);
                }
            }

        } else {
            double* tmppts = new double[iTotalBlockSize * iDim];
            fseek(fdMesh, iRealHeaderSize +
                (long)nFloatsInDomain * sizeof(double) * block,
                SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), iTotalBlockSize * iDim, fdMesh); (void)res;
            if (bSwapEndian)
                ByteSwapArray(tmppts, iTotalBlockSize * iDim);
            for (size_t i = 0; i < iTotalBlockSize; i++) {
                x[i] = static_cast<int>(tmppts[i]);
            }
            for (size_t i = 0; i < iTotalBlockSize; i++) {
                y[i] = static_cast<int>(tmppts[i + iTotalBlockSize]);
            }
            if (iDim == 3) {
                for (size_t i = 0; i < iTotalBlockSize; i++) {
                    z[i] = static_cast<int>(tmppts[i + 2 * iTotalBlockSize]);
                }
            }
            delete[] tmppts;
        }
    } else {
        for (int ii = 0; ii < iTotalBlockSize; ii++) {
            fseek(fdMesh, (long)iAsciiMeshFileStart +
                (long)block * iAsciiMeshFileLineLen * iTotalBlockSize +
                (long)ii * iAsciiMeshFileLineLen, SEEK_SET);
            if (iDim == 3) {
                int res = fscanf(fdMesh, " %f %f %f", &x[ii], &y[ii], &z[ii]); (void)res;
            } else {
                int res = fscanf(fdMesh, " %f %f", &x[ii], &y[ii]); (void)res;
                memset(z, 0, iTotalBlockSize * sizeof(float));
            }
        }
    }
    return true;
}

bool ReadNek::ReadVelocity(int timestep, int block, float* x, float* y, float* z) {


    if (timestep < 0) {
        return false;
    }

    int ii;
    //ReadBlockLocations();
    if (timestep != iCurrTimestep || (bParFormat && aBlockLocs[block * 2] != iCurrVarProc)) {
        if (fdVar)
            fclose(fdVar);

        iCurrVarProc = 0;
        if (bParFormat)
            iCurrVarProc = aBlockLocs[block * 2];

        string filename = GetFileName(timestep, iCurrVarProc);

        fdVar = fopen(filename.c_str(), "rb");
        if (!fdVar) {
            sendError(".nek5000 could not open file " + filename);
            return false;
        }

        iCurrTimestep = timestep;
        if (!bBinary)
            FindAsciiDataStart(fdVar, iAsciiCurrFileStart, iAsciiCurrFileLineLen);
    }

    
    DomainParams dp = GetDomainSizeAndVarOffset(timestep, "velocity");
    if (bParFormat)
        block = aBlockLocs[block * 2 + 1];

    long iRealHeaderSize = iHeaderSize + (bParFormat ? vBlocksPerFile[iCurrVarProc] * sizeof(int) : 0);

    if (bBinary) {
        long filepos;
        if (!bParFormat)
            filepos = (long)iRealHeaderSize + (long)(dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else
            //This assumes [block 0: 216u 216v 216w][block 1: 216u 216v 216w]...[block n: 216u 216v 216w]
            filepos = (long)iRealHeaderSize +
            (long)vBlocksPerFile[iCurrVarProc] * dp.varOffsetBinary * iPrecision + //the header and mesh if one exists
            (long)block * iTotalBlockSize * iDim * iPrecision;
        if (iPrecision == 4) {
            fseek(fdVar, filepos, SEEK_SET);
            size_t res = fread(x, sizeof(float), iTotalBlockSize, fdVar); (void)res;
            res = fread(y, sizeof(float), iTotalBlockSize, fdVar); (void)res;
            if (iDim == 3) {
                res = fread(z, sizeof(float), iTotalBlockSize, fdVar); (void)res;
            }
            else {
                memset(z, 0, iTotalBlockSize * sizeof(float));
            }
            //for (size_t i = 0; i < iTotalBlockSize; i++) {
            //    float v = sqrt()
            //}
            if (bSwapEndian) {
                ByteSwapArray(x, iTotalBlockSize);
                ByteSwapArray(y, iTotalBlockSize);
                if (iDim == 3) {
                    ByteSwapArray(z, iTotalBlockSize);
                }
            }
        } else {
            double* tmppts = new double[iTotalBlockSize * iDim];
            fseek(fdVar, filepos, SEEK_SET);
            size_t res = fread(tmppts, sizeof(double), iTotalBlockSize * iDim, fdVar); (void)res;

            if (bSwapEndian)
                ByteSwapArray(tmppts, iTotalBlockSize * iDim);

            for (ii = 0; ii < iTotalBlockSize; ii++) {
                x[ii] = (double)tmppts[ii];
                y[ii] = (double)tmppts[ii + iTotalBlockSize];
                if (iDim == 3) {
                    z[ii] = (double)tmppts[ii + iTotalBlockSize + iTotalBlockSize];
                } else {
                    z[ii] = 0.0;
                }
            }
            delete[] tmppts;
        }
    } else {
        for (ii = 0; ii < iTotalBlockSize; ii++) {
            fseek(fdVar, (long)iAsciiCurrFileStart +
                (long)block * iAsciiCurrFileLineLen * iTotalBlockSize +
                (long)ii * iAsciiCurrFileLineLen +
                (long)dp.varOffsetAscii, SEEK_SET);
            if (iDim == 3) {
                int res = fscanf(fdVar, " %f %f %f", x + ii, y +ii, z+ ii); (void)res;
            } else {
                int res = fscanf(fdVar, " %f %f", &x[ii], &y[ii]); (void)res;
                z[ii] = 0.0f;
            }
        }
    }
    return true;
}

bool ReadNek::ReadVar(const char* varname, int timestep, int block, float* data) {

    //ReadBlockLocations();

    if (timestep != iCurrTimestep || (bParFormat && aBlockLocs[block * 2] != iCurrVarProc)) {
        if (fdVar)
            fclose(fdVar);



        iCurrVarProc = 0;
        if (bParFormat)
            iCurrVarProc = aBlockLocs[block * 2];

        string filename = GetFileName(timestep, iCurrVarProc);

        fdVar = fopen(filename.c_str(), "rb");
        if (!fdVar)             {
            sendError(".nek500: can not open file" + filename);
            return false;
        }



        iCurrTimestep = timestep;
        if (!bBinary)
            FindAsciiDataStart(fdVar, iAsciiCurrFileStart, iAsciiCurrFileLineLen);
    }


    DomainParams dp = GetDomainSizeAndVarOffset(timestep, varname);

    if (bParFormat)
        block = aBlockLocs[block * 2 + 1];

    long iRealHeaderSize = iHeaderSize + (bParFormat ? vBlocksPerFile[iCurrVarProc] * sizeof(int) : 0);

    if (bBinary) {
        long filepos;
        if (!bParFormat)
            filepos = (long)iRealHeaderSize + ((long)dp.domSizeInFloats * block + dp.varOffsetBinary) * sizeof(float);
        else {
            // This assumes uvw for all fields comes after the mesh as [block0: 216u 216v 216w]...
            // then p or t as   [block0: 216p][block1: 216p][block2: 216p]...
            if (strcmp(varname + 2, "velocity") == 0) {
                filepos = (long)iRealHeaderSize +                              //header
                    (long)dp.timestepHasMesh * vBlocksPerFile[iCurrVarProc] * iTotalBlockSize * iDim * iPrecision + //mesh
                    (long)block * iTotalBlockSize * iDim * iPrecision +                  //start of block
                    (long)(varname[0] - 'x') * iTotalBlockSize * iPrecision;            //position within block
            } else
                filepos = (long)iRealHeaderSize +
                (long)vBlocksPerFile[iCurrVarProc] * dp.varOffsetBinary * iPrecision + //the header, mesh, vel if present,
                (long)block * iTotalBlockSize * iPrecision;
        }
        if (iPrecision == 4) {
            fseek(fdVar, filepos, SEEK_SET);
            size_t res = fread(data, sizeof(float), iTotalBlockSize, fdVar); (void)res;
            if (bSwapEndian)
                ByteSwapArray(data, iTotalBlockSize);
        } else {
            double* tmp = new double[iTotalBlockSize];

            fseek(fdVar, filepos, SEEK_SET);
            size_t res = fread(tmp, sizeof(double), iTotalBlockSize, fdVar); (void)res;
            if (bSwapEndian)
                ByteSwapArray(tmp, iTotalBlockSize);

            for (int ii = 0; ii < iTotalBlockSize; ii++)
                data[ii] = (float)tmp[ii];

            delete[] tmp;
        }
    } else {
        float* var_tmp = data;
        for (int ii = 0; ii < iTotalBlockSize; ii++) {
            fseek(fdVar, (long)iAsciiCurrFileStart +
                (long)block * iAsciiCurrFileLineLen * iTotalBlockSize +
                (long)ii * iAsciiCurrFileLineLen +
                (long)dp.varOffsetAscii, SEEK_SET);
            int res = fscanf(fdVar, " %f", var_tmp); (void)res;
            var_tmp++;
        }
    }
    return true;
}

bool ReadNek::ReadScalarData(Reader::Token &token, vistle::Port *p, const std::string& varname, int timestep, int partition) {

    int blocksRead = partition * std::floor(iNumBlocks / p_numPartitions->getValue());
    auto grid = mGrids.find(partition);
    if (grid == mGrids.end()) {
        sendError(".nek5000 did not find a matching grid for block %d", partition);
        return false;
    }
    Vec<Scalar>::ptr scal(new Vec<Scalar>(iTotalBlockSize * BlocksToRead(partition)));

    float* data = scal->x().data();
    scal->setGrid(grid->second);
    scal->setTimestep(timestep);
    scal->setBlock(partition);
    scal->setMapping(false ? DataBase::Element : DataBase::Vertex);
    scal->addAttribute("_species", varname);

    for (size_t b = 0; b < BlocksToRead(partition); b++) {
        if (!ReadVar(varname.c_str(), timestep, blocksRead + b, data + b * iTotalBlockSize)) {
            return false;
        }
    }
    token.addObject(p, scal);
    return true;
}

void ReadNek::ReadBlockLocations() {
    // In each parallel file, in the header, there's a table that maps 
    // each local block to a global id which starts at 1.  Here, I make 
    // an inverse map, from a zero-based global id to a proc num and local
    // offset.

    if (!bBinary || !bParFormat)
        return;

    int ii, jj;
    //aBlockLocs = new int[2 * iNumBlocks];
    vBlocksPerFile = std::vector<int>(iNumOutputDirs);
    std::fill(vBlocksPerFile.begin(), vBlocksPerFile.end(), 0);
    int iRank = 0, nProcs = 1;
#ifdef PARALLEL
    MPI_Comm_rank(VISIT_MPI_COMM, &iRank);
    MPI_Comm_size(VISIT_MPI_COMM, &nProcs);
#endif
    aBlockLocs = std::vector<int>(2 * iTotalNumBlocks);
    std::fill(aBlockLocs.begin(), aBlockLocs.end(), 0);


    string blockfilename;
    int* tmpBlocks = new int[iTotalNumBlocks];

    ifstream f;
    int badFile = iTotalNumBlocks + 1;
    for (ii = iRank; ii < iNumOutputDirs; ii += nProcs) {
        blockfilename = GetFileName(0, ii);
        f.open(blockfilename, ifstream::binary);
        if (!f.is_open()) {
            badFile = ii;
            break;
        }

        int tmp1, tmp2, tmp3, tmp4;
        f.seekg(5, std::ios_base::beg);  //seek past the #std
        f >> tmp1 >> tmp2 >> tmp3 >> tmp4 >> vBlocksPerFile[ii];

#ifndef USE_SIMPLE_BLOCK_NUMBERING
        f.seekg(136, std::ios_base::beg);
        f.read((char*)tmpBlocks, vBlocksPerFile[ii] * sizeof(int));
        if (bSwapEndian)
            ByteSwapArray(tmpBlocks, vBlocksPerFile[ii]);

        for (jj = 0; jj < vBlocksPerFile[ii]; jj++) {
            int iBlockID = tmpBlocks[jj] - 1;

            if (iBlockID < 0 ||
                iBlockID >= iTotalNumBlocks ||
                aBlockLocs[iBlockID * 2] != 0 ||
                aBlockLocs[iBlockID * 2 + 1] != 0) {
                sendError(" .nek500: Error reading parallel file block IDs.");
            }
            aBlockLocs[iBlockID * 2] = ii;
            aBlockLocs[iBlockID * 2 + 1] = jj;
        }
#endif
        f.close();
    }

    //badFile = UnifyMinimumValue(badFile);
    if (badFile < iTotalNumBlocks) {
        blockfilename = GetFileName(0, badFile);
        sendError("Could not open file \"%s\" to read block locations.", blockfilename.c_str());
    }
    delete[] tmpBlocks;

#ifdef PARALLEL
    int* aTmpBlocksPerFile = new int[iNumOutputDirs];

    MPI_Allreduce(aBlocksPerFile, aTmpBlocksPerFile, iNumOutputDirs,
        MPI_INT, MPI_BOR, VISIT_MPI_COMM);
    delete[] aBlocksPerFile;
    aBlocksPerFile = aTmpBlocksPerFile;
#endif

    //Do a sanity check
    int sum = 0;

    for (ii = 0; ii < iNumOutputDirs; ii++)
        sum += vBlocksPerFile[ii];

    if (sum != iTotalNumBlocks) {
        sendError(".nek5000: Sum of blocks per file does not equal total number of blocks");
    }

#ifdef USE_SIMPLE_BLOCK_NUMBERING
    //fill in aBlockLocs ...
    int* p = aBlockLocs;
    for (jj = 0; jj < iNumOutputDirs; jj++) {
        for (ii = 0; ii < aBlocksPerFile[jj]; ii++) {
            *p++ = jj;
            *p++ = ii;
        }
    }
#else
#ifdef PARALLEL
    int* aTmpBlockLocs = new int[2 * iNumBlocks];

    MPI_Allreduce(aBlockLocs, aTmpBlockLocs, 2 * iNumBlocks,
        MPI_INT, MPI_BOR, VISIT_MPI_COMM);
    delete[] aBlockLocs;
    aBlockLocs = aTmpBlockLocs;
#endif
#endif
}

void ReadNek::UpdateCyclesAndTimes() {
    if (aTimes.size() != (size_t)iNumTimesteps) {
        aTimes.resize(iNumTimesteps);
        aCycles.resize(iNumTimesteps);
        vTimestepsWithMesh.resize(iNumTimesteps, false);
        vTimestepsWithMesh[0] = true;
        readTimeInfoFor.resize(iNumTimesteps, false);
    }
    ifstream f;
    char dummy[64];
    double t;
    int    c;
    string v;
    t = 0.0;
    c = 0;

    string meshfilename = GetFileName(curTimestep, 0);
    f.open(meshfilename.c_str());

    if (!bParFormat) {
        string tString, cString;
        f >> dummy >> dummy >> dummy >> dummy >> tString >> cString >> v;  //skip #blocks and block size
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

    aTimes[curTimestep] = t;
    aCycles[curTimestep] = c;
    //if (metadata != NULL) {
    //    metadata->SetTime(curTimestep + timeSliceOffset, t);
    //    metadata->SetTimeIsAccurate(true, curTimestep + timeSliceOffset);
    //    metadata->SetCycle(curTimestep + timeSliceOffset, c);
    //    metadata->SetCycleIsAccurate(true, curTimestep + timeSliceOffset);
    //}

    // If this file contains a mesh, the first variable codes after the 
    // cycle number will be X Y
    if (v.find("X") != string::npos)
        vTimestepsWithMesh[curTimestep] = true;

    // Nek has a bug where the time and cycle sometimes run together (e.g. 2.52000E+0110110 for
    // time 25.2, cycle 10110).  If that happens, then v will be Y
    if (v.find("Y") != string::npos)
        vTimestepsWithMesh[curTimestep] = true;

    readTimeInfoFor[curTimestep] = true;
}

std::string ReadNek::GetFileName(int rawTimestep, int pardir) {
    int timestep = rawTimestep + iFirstTimestep;
    int nPrintfTokens = 0;

    for (size_t ii = 0; ii < fileTemplate.size() - 1; ii++) {

        if (fileTemplate[ii] == '%' && fileTemplate[ii + 1] != '%')
            nPrintfTokens++;
    }

    if (nPrintfTokens > 1) {
        bBinary = true;
        bParFormat = true;
    }

    if (!bParFormat && nPrintfTokens != 1) {
        sendError("Nek: The filetemplate tag must receive only one printf token for serial Nek files.");
    } else if (bParFormat && (nPrintfTokens < 2 || nPrintfTokens > 3)) {
        sendError("Nek: The filetemplate tag must receive either 2 or 3 printf tokens for parallel Nek files.");
    }
    int bufSize = fileTemplate.size();
    int len;
    char* outFileName = nullptr;
    do
    {
        bufSize += 64;
        outFileName = new char[bufSize];
        if (!bParFormat)
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), timestep);
        else if (nPrintfTokens == 2)
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), pardir, timestep);
        else
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), pardir, pardir, timestep);
    } while (len >= bufSize);
    string s(outFileName, len);
    delete[]outFileName;
    return string(s);
}

void ReadNek::FindAsciiDataStart(FILE* fd, int& outDataStart, int& outLineLen) {
    //Skip the header, then read a float for each block.  Then skip beyond the
//newline character and return the current position.
    fseek(fd, iHeaderSize, SEEK_SET);
    for (int ii = 0; ii < iTotalNumBlocks; ii++) {
        float dummy;
        int res = fscanf(fd, " %f", &dummy); (void)res;
    }
    char tmp[1024];
    char* res = NULL; (void)res;
    res = fgets(tmp, 1023, fd);
    outDataStart = ftell(fd);

    res = fgets(tmp, 1023, fd);
    outLineLen = ftell(fd) - outDataStart;
}

void ReadNek::makeConectivityList(Index* connectivityList, Index numBlocks) {

    Index* nl = connectivityList;
    for (Index elements_so_far = 0; elements_so_far < numBlocks; ++elements_so_far) {
        long pt_start = iTotalBlockSize * elements_so_far;
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
}

int ReadNek::BlocksToRead(int partition) {
    int blocksToRead = iNumBlocks / p_numPartitions->getValue();
    if (partition == p_numPartitions->getValue() - 1) {
        blocksToRead += iNumBlocks % p_numPartitions->getValue(); //the last one has to read the rest
    }
    return blocksToRead;
}



ReadNek::DomainParams ReadNek::GetDomainSizeAndVarOffset(int iTimestep, const char* var) {
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
    nFloatsPerSample += iNumSFields;

    params.domSizeInFloats = nFloatsPerSample * iTotalBlockSize;

    if (var) {
        int iNumPrecedingFloats = 0;
        if (STREQUAL(var, "velocity") == 0 ||
            STREQUAL(var, "velocity_mag") == 0 ||
            STREQUAL(var, "x_velocity") == 0) {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;
        } else if (STREQUAL(var, "y_velocity") == 0) {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;

            iNumPrecedingFloats += 1;
        } else if (STREQUAL(var, "z_velocity") == 0) {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;

            iNumPrecedingFloats += 2;
        } else if (STREQUAL(var, "pressure") == 0) {
            if (params.timestepHasMesh)
                iNumPrecedingFloats += iDim;
            if (bHasVelocity)
                iNumPrecedingFloats += iDim;
        } else if (STREQUAL(var, "temperature") == 0) {
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

            int iSField = atoi(var + 1);
            //iSField should be between 1..iNumSFields, inclusive
            iNumPrecedingFloats += iSField - 1;
        }
        params.varOffsetBinary = iTotalBlockSize * iNumPrecedingFloats;
        params.varOffsetAscii = 14 * iNumPrecedingFloats;
    }
    return params;
}

ReadNek::ReadNek(const std::string& name, int moduleID, mpi::communicator comm)
   :vistle::Reader("Read .nek5000 files", name, moduleID, comm)
{
   // Output ports
   p_grid = createOutputPort("grid_out", "grid");
   p_velocity = createOutputPort("velosity_out", "velocity");
   p_pressure = createOutputPort("pressure_out", "pressure data");
   p_temperature = createOutputPort("temperature_out", "temperature data");
   
   // Parameters
   p_data_path = addStringParameter("filename", "Geometry file path", "", Parameter::Filename);
   //p_byteswap = addIntParameter("byteswap", "Perform Byteswapping", Auto, Parameter::Choice);
   //V_ENUM_SET_CHOICES(p_byteswap, ByteSwap);

   p_only_geometry = addIntParameter("OnlyGeometry", "Reading only Geometry? yes|no", false, Parameter::Boolean);
   //p_readOptionToGetAllTimes = addIntParameter("ReadOptionToGetAllTimes", "Read all times and cycles", false, Parameter::Boolean);
   //p_duplicateData = addIntParameter("DublicateData", "Duplicate data for particle advection(slower for all other techniques)", false, Parameter::Boolean);
   p_numBlocks = addIntParameter("number of blocks", "number of blocks, <= 0 to read all", 0);
   p_numPartitions = addIntParameter("numerOfPartitions", "number of partitions", 1);


   observeParameter(p_data_path);
   //observeParameter(p_byteswap);
   observeParameter(p_only_geometry);
   //observeParameter(p_readOptionToGetAllTimes);
   //observeParameter(p_duplicateData);
   observeParameter(p_numBlocks);
   observeParameter(p_numPartitions);

   setParallelizationMode(ParallelizationMode::ParallelizeBlocks);

   MPI_Comm_size(comm, &iNumberOfRanks);

   vistle::IntParameter* test = addIntParameter("test", "test", 1);


}

ReadNek::~ReadNek() {
}


MODULE_MAIN(ReadNek)
