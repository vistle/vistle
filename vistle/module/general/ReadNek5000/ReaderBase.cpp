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
    :file(file)
    ,numPartitions(numPartitions)
    ,numBlocksToRead(blocksToRead)
{
}

ReaderBase::~ReaderBase()
{

}

bool ReaderBase::init()
{
    if (!parseMetaDataFile() || !ParseNekFileHeader() || !ParseGridMap()) {
        return false;
    }
    if(numBlocksToRead < 1 || numBlocksToRead > totalNumBlocks)
        numBlocksToRead = totalNumBlocks;
    UpdateCyclesAndTimes();
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
    return numTimesteps;
}

size_t ReaderBase::getNumScalarFields() const
{
    return numScalarFields;
}

int ReaderBase::getDim() const
{
    return dim;
}

bool ReaderBase::hasVelocity() const
{
    return bHasVelocity;
}

bool ReaderBase::hasPressure() const
{
    return bHasPressure;
}

bool ReaderBase::hasTemperature() const
{
    return bHasTemperature;
}

//protected methods
std::string ReaderBase::GetFileName(int rawTimestep, int pardir) {
    int timestep = rawTimestep + firstTimestep;
    int nPrintfTokens = 0;

    for (size_t ii = 0; ii < fileTemplate.size() - 1; ii++) {

        if (fileTemplate[ii] == '%' && fileTemplate[ii + 1] != '%')
            nPrintfTokens++;
    }

    if (nPrintfTokens > 1) {
        isBinary = true;
        isParallelFormat = true;
    }

    if (!isParallelFormat && nPrintfTokens != 1) {
        sendError("Nek: The filetemplate tag must receive only one printf token for serial Nek files.");
    } else if (isParallelFormat && (nPrintfTokens < 2 || nPrintfTokens > 3)) {
        sendError("Nek: The filetemplate tag must receive either 2 or 3 printf tokens for parallel Nek files.");
    }
    int bufSize = fileTemplate.size();
    int len;
    string s;
    do
    {
        bufSize += 64;
        char *outFileName = new char[bufSize];
        if (!isParallelFormat)
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), timestep);
        else if (nPrintfTokens == 2)
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), pardir, timestep);
        else
            len = snprintf(outFileName, bufSize, fileTemplate.c_str(), pardir, pardir, timestep);
        s = string(outFileName, len);
        delete[]outFileName;
    } while (len >= bufSize);
    return s;
}

void ReaderBase::sendError(const std::string &msg)
{
    cerr << msg << endl;
}

void ReaderBase::UpdateCyclesAndTimes() {
    if (aTimes.size() != (size_t)numTimesteps) {
        aTimes.resize(numTimesteps);
        aCycles.resize(numTimesteps);
        vTimestepsWithMesh.resize(numTimesteps, false);
        vTimestepsWithMesh[0] = true;
        readTimeInfoFor.resize(numTimesteps, false);
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

    if (!isParallelFormat) {
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
    //if (metadata != nullptr) {
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
//private methods
void ReaderBase::setAllEdgesInCornerIndices() {
    allEdgesInCornerIndices.insert({ 1,2 });
    allEdgesInCornerIndices.insert({ 1,3 });
    allEdgesInCornerIndices.insert({ 2,4 });
    allEdgesInCornerIndices.insert({ 3,4 });
    if (dim == 3) {
        allEdgesInCornerIndices.insert({ 1,5 });
        allEdgesInCornerIndices.insert({ 2,6 });
        allEdgesInCornerIndices.insert({ 3,7 });
        allEdgesInCornerIndices.insert({ 4,8 });
        allEdgesInCornerIndices.insert({ 5,6 });
        allEdgesInCornerIndices.insert({ 5,7 });
        allEdgesInCornerIndices.insert({ 6,8 });
        allEdgesInCornerIndices.insert({ 7,8 });
    }
}

void ReaderBase::setAllPlanesInCornerIndices() {
    vector < Plane> planes;
    allPlanesInCornerIndices.insert({ 1, 2, 3, 4 });
    allPlanesInCornerIndices.insert({ 1, 2, 5, 6 });
    allPlanesInCornerIndices.insert({ 1, 3, 5, 7 });
    allPlanesInCornerIndices.insert({ 2, 4, 6, 8 });
    allPlanesInCornerIndices.insert({ 3, 4, 7, 8 });
    allPlanesInCornerIndices.insert({ 5, 6, 7, 8 });


}

void ReaderBase::setBlockIndexToConnectivityIndex() {
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



bool ReaderBase::parseMetaDataFile() {
    string tag;
    char buf[2048];
    ifstream  f(file);
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
            string  dummy_endianness;
            f >> dummy_endianness;
        } else if (tag == "filetemplate:") {
            f >> fileTemplate;
        } else if (tag == "firsttimestep:") {
            f >> firstTimestep;
        } else if (tag == "numtimesteps:") {
            f >> numTimesteps;
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
                isBinary = true;
            } else if (tag == "binary6") {
                isBinary = true;
                isParallelFormat = true;
            } else if (tag == "ascii") {
                isBinary = false;
            } else {
                sendError(".nek5000: Value following \"type\" must be \"ascii\" or \"binary\" or \"binary6\"");
                if (f.is_open())
                    f.close();
                return false;
            }
        } else if (tag == "numoutputdirs:") {
            //This tag can be deprecated, because the number of output dirs
            //can be determined from the header, and the parallel nature of the
            //file can be inferred from the number of printf tokens in the template.
            //This reader scans the headers for the number of fld files
            f >> numOutputDirs;
            if (numOutputDirs > 1)
                isParallelFormat = true;
        } else if (tag == "timeperiods:") {
            f >> numberOfTimePeriods;
        } else if (tag == "gapBetweenTimePeriods:") {
            f >> gapBetweenTimePeriods;
        } else if (tag == "NEK3D") {
            //This is an obsolete tag, ignore it.
        } else if (tag == "version:") {
            //This is an obsolete tag, ignore it, skipping the version number
            //as well.
            string version;
            f >> version;
        } else {
            sendError(".nek5000: Error parsing file.  Unknown tag " + tag);
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

bool ReaderBase::ParseGridMap() {
    string base_filename = file;
    size_t ext = base_filename.find_last_of('.') + 1;
    base_filename.erase(ext);
    string map_filename = base_filename + "map";
    ifstream  mptr(map_filename);
    if (mptr.is_open()) {

        for (size_t i = 0; i < 7; i++) {
            mptr >> mapFileHeader[i];
        }
        //std::vector<int> map_elements(mapFileHeader[0]);
        mapFileData.reserve(mapFileHeader[0]);
        int columns = dim == 2 ? 5 : 9;
        for (int i = 0; i < mapFileHeader[0]; ++i) {
            array<int, 9> line;
            for (size_t j = 0; j < columns; j++) {
                mptr >> line[j];
            }
            mapFileData.emplace_back(line);
            //map_elements[i] = line[0] + 1;
        }
        mptr.close();
        //map<int, vector<int>> matches;
        //int line = 1;
        //for (auto entry : mapFileData) {
        //    for (size_t i = 1; i <= numCorners; i++) {
        //        matches[entry[i]].push_back(line);
        //    }
        //    ++line;
        //}


        return true;
    }
    string ma2_filename = base_filename + "ma2";
    ifstream  ma2ptr(ma2_filename, ifstream::binary);
    if (ma2ptr.is_open())
    {
        //ma2ptr.seekg(0, ios::beg);
        cerr << "ma2 file is open" << endl;
        string version;
        ma2ptr >> version;
        if (version != "#v001")
        {
            sendError("ma2 file of version " + version + " is not supported");
            return false;
        }
        cerr << "version: " << version << endl;
        for (size_t i = 0; i < 7; i++) {
            ma2ptr >> mapFileHeader[i];
            cerr << mapFileHeader[i] << " ";
        }
        cerr << endl;
        ma2ptr.seekg(132/4 * sizeof(float), ios::beg);
        char test;
        ma2ptr >> test;
        if (test)
        {
            cerr << "test = true" << endl;
        }
        else {
            cerr  << "test = false " << endl;
        }
        ma2ptr.seekg((132 / 4  + 1) * sizeof(float), ios::beg);
        mapFileData.reserve(mapFileHeader[0]);
        int columns = dim == 2 ? 5 : 9;
        float number = 66;


        int* map = new int[columns * mapFileHeader[0]];
        ma2ptr.read((char*)map, columns * mapFileHeader[0] * sizeof(float));

        for (int i = 0; i < mapFileHeader[0]; ++i) {
            array<int, 9> line;
            cerr << " line " << i << ": ";
            for (size_t j = 0; j < columns; j++) {
                line[j] = map[i * columns + j];

                //ma2ptr >> line[j];
                cerr << line[j] << " ";
            }
            cerr << endl;
            mapFileData.emplace_back(line);
            //map_elements[i] = line[0] + 1;
        } 
        delete[] map;
        ma2ptr.close();
        return true;




    }

    sendError("ReadNek: can neither open map file " + map_filename + " nor ma2 file " + ma2_filename );
    return false;
}

bool ReaderBase::ParseNekFileHeader() {
    string buf2, tag;

    //Now read the header of one of the files to get block and variable info
    string blockfilename = GetFileName(0, 0);
    ifstream  f(blockfilename, ifstream::binary);

    if (!f.is_open()) {
        sendError("Could not open file " + file + ", which should exist according to header file " + blockfilename + ".");
        return false;
    }

    // Determine the type (ascii or binary)
    // Parallel type is determined by the number of tokens in the file template
    // and is always binary
    if (!isParallelFormat) {
        float test;
        f.seekg(80, ios::beg);

        f.read((char*)(&test), 4);
        if (test > 6.5 && test < 6.6)
            isBinary = true;
        else {
            ByteSwapArray(&test, 1);
            if (test > 6.5 && test < 6.6)
                isBinary = true;
        }
        f.seekg(0, ios::beg);
    }

    //iHeaderSize no longer includes the size of the block index metadata, for the
    //parallel format, since this now can vary per file.
    if (isBinary && isParallelFormat)
        iHeaderSize = 136;
    else if (isBinary && !isParallelFormat)
        iHeaderSize = 84;
    else
        iHeaderSize = 80;


    if (!isParallelFormat) {
        f >> totalNumBlocks;
        f >> blockDimensions[0];
        f >> blockDimensions[1];
        f >> blockDimensions[2];

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
        f >> blockDimensions[0];
        f >> blockDimensions[1];
        f >> blockDimensions[2];
        f >> buf2;  //blocks per file
        f >> totalNumBlocks;

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
        numOutputDirs = 0;
        while (f.peek() >= '0' && f.peek() <= '9') {
            numOutputDirs *= 10;
            numOutputDirs += (f.get() - '0');
        }

        ParseFieldTags(f);

    }
    dim = blockDimensions[2] == 1 ? 2 : 3;
    numCorners = dim == 2 ? 4 : 8;
    blockSize = blockDimensions[0] * blockDimensions[1] * blockDimensions[2];
    hexesPerBlock = (blockDimensions[0] - 1) * (blockDimensions[1] - 1);
    if (dim == 3)
        hexesPerBlock *= (blockDimensions[2] - 1);
    if (isBinary) {
        // Determine endianness and whether we need to swap bytes.
        // If this machine's endian matches the file's, the read will
        // put 6.54321 into this float.

        float test;
        if (!isParallelFormat) {
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
    return true;
}

void ReaderBase::ParseFieldTags(ifstream& f) {
    numScalarFields = 1;
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
            numScalarFields = c - '0';
        } else if (c == 'S') {
            while (f.peek() == ' ')
                f.get();
            char digit1 = f.get();
            while (f.peek() == ' ')
                f.get();
            char digit2 = f.get();

            if (digit1 >= '0' && digit1 <= '9' &&
                digit2 >= '0' && digit2 <= '9')
                numScalarFields = (digit1 - '0') * 10 + (digit2 - '0');
            else
                numScalarFields = 1;
        } else
            break;
    }
    if (!foundCoordinates) {
        sendError("Nek: The first time step in a Nek file must contain a mesh");

    }
}

void ReaderBase::makeBaseConnList() {
    using vistle::Index;
    baseConnList.resize(numCorners * hexesPerBlock);
    Index* nl = baseConnList.data();
    for (Index ii = 0; ii < blockDimensions[0] - 1; ii++) {
        for (Index jj = 0; jj < blockDimensions[1] - 1; jj++) {
            if (dim == 2) {
                *nl++ = jj * (blockDimensions[0]) + ii;
                *nl++ = jj * (blockDimensions[0]) + ii + 1;
                *nl++ = (jj + 1) * (blockDimensions[0]) + ii + 1;
                *nl++ = (jj + 1) * (blockDimensions[0]) + ii;
            } else {
                for (Index kk = 0; kk < blockDimensions[2] - 1; kk++) {
                    *nl++ = kk * (blockDimensions[1]) * (blockDimensions[0]) + jj * (blockDimensions[0]) + ii;
                    *nl++ = kk * (blockDimensions[1]) * (blockDimensions[0]) + jj * (blockDimensions[0]) + ii + 1;
                    *nl++ = kk * (blockDimensions[1]) * (blockDimensions[0]) + (jj + 1) * (blockDimensions[0]) + ii + 1;
                    *nl++ = kk * (blockDimensions[1]) * (blockDimensions[0]) + (jj + 1) * (blockDimensions[0]) + ii;
                    *nl++ = (kk + 1) * (blockDimensions[1]) * (blockDimensions[0]) + jj * (blockDimensions[0]) + ii;
                    *nl++ = (kk + 1) * (blockDimensions[1]) * (blockDimensions[0]) + jj * (blockDimensions[0]) + ii + 1;
                    *nl++ = (kk + 1) * (blockDimensions[1]) * (blockDimensions[0]) + (jj + 1) * (blockDimensions[0]) + ii + 1;
                    *nl++ = (kk + 1) * (blockDimensions[1]) * (blockDimensions[0]) + (jj + 1) * (blockDimensions[0]) + ii;
                }
            }
        }
    }
}
}
