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
#ifndef _READNEK5000_H
#define _READNEK5000_H

#include <util/coRestraint.h>
#include <util/byteswap.h>

#include <core/vec.h>
//#include <core/structuredgrid.h>
#include <core/unstr.h>
#include<core/index.h>
#include <module/reader.h>

#include <vector>
#include <map>
#include <string>
#include <iostream>    
#include <fstream>
#include <cstdio>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ByteSwap, (Off)(On)(Auto))

typedef struct {
    std::string var;
    int         element;
    int         timestep;
} PointerKey;

class     avtIntervalTree;

class KeyCompare {
public:
    bool operator()(const PointerKey& x, const PointerKey& y) const {
        if (x.element != y.element)
            return (x.element > y.element);
        if (x.timestep != y.timestep)
            return (x.timestep > y.timestep);
        return (x.var > y.var);
    }
};

class NekReaderBase;
class ReadNek : public vistle::Reader {
public:
private:
    bool prepareRead() override;
    bool read(Token& token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter* param) override;
    bool finishRead() override;

//      This method is called as part of initialization.  It parses the text
//      file which is a companion to the series of .fld files that make up a
//      dataset.
    bool parseMetaDataFile();
//      This method is called as part of initialization.  Some of the file
//      metadata is written in the header of each timestep, and this method
//      reads and parses that metadata.
    bool ParseNekFileHeader();
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

    //      Gets the mesh associated with this file.  The mesh is returned as a
//      derived type of vtkDataSet (ie vtkRectilinearGrid, vtkStructuredGrid,
//      vtkUnstructuredGrid, etc).
    bool ReadMesh(int numReads, int timestep, int element, float* x, float* y, float* z);
    //read velocity in x, y and z
    bool ReadVelocity(int numReads, int timestep, int block, float* x, float* y, float* z);
    //read var with varname in data
    bool ReadVar(int numReads, const char* varname, int timestep, int block, float* data);
    
    bool ReadScalarData(Token &token, vistle::Port *p, const std::string& varname, int timestep, int partition);
    void ReadBlockLocations();
    void UpdateCyclesAndTimes();
//      Create a filename from the template and other info.
    std::string GetFileName(int rawTimestep, int pardir);
    //      If the data is ascii format, there is a certain amount of deprecated
//      data to skip at the beginning of each file.  This method tells where to
//      seek to, to read data for the first block.
    void FindAsciiDataStart(FILE* fd, int& outDataStart, int& outLineLen);
    void makeConectivityList(vistle::Index* connectivityList, vistle::Index numBlocks);
    int BlocksToRead(int partition);
    struct DomainParams {
        int domSizeInFloats = 0;
        int varOffsetBinary = 0;
        int varOffsetAscii = 0;
        bool timestepHasMesh = 0;
    };
    DomainParams GetDomainSizeAndVarOffset(int iTimestep, const char* varn);
    template<class T>
    void ByteSwapArray(T* val, int size)
    {
        for (int i = 0; i < size; i++)
        {
            val[i] = vistle::byte_swap<vistle::endianness::little_endian, vistle::endianness::big_endian>(val[i]);
        }
    }

    // Ports
    vistle::Port* p_grid = nullptr;
    vistle::Port* p_velocity = nullptr;
    vistle::Port* p_pressure = nullptr;
    vistle::Port* p_temperature = nullptr;

    std::vector<vistle::Port*> pv_misc;
    // Parameters
    vistle::StringParameter* p_data_path = nullptr;
    vistle::IntParameter* p_only_geometry = nullptr;
    vistle::IntParameter* p_byteswap = nullptr;
    vistle::IntParameter* p_readOptionToGetAllTimes = nullptr;
    vistle::IntParameter* p_duplicateData = nullptr;
    vistle::IntParameter* p_numBlocks = nullptr;
    vistle::IntParameter* p_numPartitions = nullptr;
    float minVeclocity = 0, maxVelocity = 0;
    int numReads = 0;
    // This info is embedded in the .nek3d text file 
        // originally specified by Dave Bremer
    std::string fileTemplate;
    int iFirstTimestep = 1;
    int iNumTimesteps = 1;
    bool bBinary = false;         //binary or ascii
    int iNumOutputDirs = 0;  //used in parallel format
    bool bParFormat = false;

    int numberOfTimePeriods = 1;
    double gapBetweenTimePeriods = 0.0;

    // This info is embedded in, or derived from, the file header
    bool bSwapEndian = false;
    int iTotalNumBlocks = 0;
    int iNumBlocks = 0;
    vistle::Index iBlockSize[3]{ 1,1,1 };
    int iTotalBlockSize = 0;
    bool bHasVelocity = false;
    bool bHasPressure = false;
    bool bHasTemperature = false;
    int iNumSFields = 0;
    int iHeaderSize = 0;
    int iDim = 3;
    int iPrecision = 4; //4 or 8 for float or double
    //only used in parallel binary
    std::vector<int> vBlocksPerFile;
    int iNumberOfRanks = 1;
    // This info is distributed through all the dumps, and only
    // computed on demand
    std::vector<int> aCycles;
    std::vector<double> aTimes;
    std::vector<bool> readTimeInfoFor;
    std::vector<bool> vTimestepsWithMesh;
    int curTimestep = 1;
    int timestepToUseForMesh = 0;




    //maps the grids to the blocks
    std::map<int, vistle::UnstructuredGrid::ptr> mGrids;
    struct OpenFile
    {
        FILE* file = nullptr;
        std::string fileName;
        int curTimestep = -1;
        int curProc = -1; //For parallel format, proc associated with file
        int  iAsciiFileStart = -1;  //For ascii data, file pos where data begins, in current timestep
        int  iAsciiFileLineLen = -1; //For ascii data, length of each line, in mesh file
    };
        // Cached data describing how to read data out of the file.
    std::map<int, OpenFile> curOpenMeshFiles, curOpenVarFiles;

    std::vector<int> aBlockLocs;           //For parallel format, make a table for looking up blocks.
                               //This has 2 ints per block, with proc # and local block #.

    // This info is for managing which blocks are read on which processors
    // and caching blocks that have been read. 
    std::vector<int>                                     myElementList;
    std::map<PointerKey, float*, KeyCompare>            cachedData;
    std::map<int, avtIntervalTree*>                     boundingBoxes;
    std::map<PointerKey, avtIntervalTree*, KeyCompare>  dataExtents;
    int                                                  cachableElementMin;
    int                                                  cachableElementMax;

public:
    ReadNek(const std::string& name, int moduleID, mpi::communicator comm);
    ~ReadNek() override;

};

#endif // _READNEK5000_H
