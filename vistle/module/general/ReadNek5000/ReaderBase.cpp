#include "ReaderBase.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>


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
    if (!parseMetaDataFile() || !ParseNekFileHeader()) {
        return false;
    }
    if(numBlocksToRead < 1 || numBlocksToRead > totalNumBlocks)
        numBlocksToRead = totalNumBlocks;
    UpdateCyclesAndTimes();

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
                isParalellFormat = true;
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
                isParalellFormat = true;
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

bool ReaderBase::ParseGridMap(){
    string map_filename = file;
    size_t ext = map_filename.find_last_of('.') + 1;
    map_filename.erase(ext);
    map_filename.insert(ext, "map");


    ifstream  mptr(map_filename);
    if(mptr.is_open())
    {
      int num_map_elements;
      string buf2;
      mptr >> num_map_elements >> buf2 >> buf2 >> buf2 >> buf2 >> buf2 >> buf2;
      std::vector<int> map_elements(num_map_elements);
      for(int i=0; i<num_map_elements; ++i)
      {
        mptr >> map_elements[i] >> buf2 >> buf2 >> buf2 >> buf2;
        if(iDim == 3)
            mptr >> buf2 >> buf2 >> buf2 >> buf2;
        map_elements[i]+=1;
      }
      mptr.close();

      all_element_list = map_elements;

      return true;
    }
return false;
}

bool ReaderBase::ParseNekFileHeader() {
    string buf2, tag;

    //Now read the header of one of the files to get block and variable info
    string blockfilename = GetFileName(0, 0);
    ifstream  f(blockfilename, ifstream::binary);

    if (!f.is_open()) {
        sendError("Could not open file " + file +", which should exist according to header file " + blockfilename + ".");
        return false;
    }

    // Determine the type (ascii or binary)
    // Parallel type is determined by the number of tokens in the file template
    // and is always binary
    if (!isParalellFormat) {
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
    if (isBinary && isParalellFormat)
        iHeaderSize = 136;
    else if (isBinary && !isParalellFormat)
        iHeaderSize = 84;
    else
        iHeaderSize = 80;


    if (!isParalellFormat) {
        f >> totalNumBlocks;
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
    if (iBlockSize[2] == 1)
        iDim = 2;
    else {
        iDim = 3;
    }
    blockSize = iBlockSize[0] * iBlockSize[1] * iBlockSize[2];
    if (isBinary) {
        // Determine endianness and whether we need to swap bytes.
        // If this machine's endian matches the file's, the read will
        // put 6.54321 into this float.

        float test;
        if (!isParalellFormat) {
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

    if (!isParalellFormat) {
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

std::string ReaderBase::GetFileName(int rawTimestep, int pardir) {
    int timestep = rawTimestep + firstTimestep;
    int nPrintfTokens = 0;

    for (size_t ii = 0; ii < fileTemplate.size() - 1; ii++) {

        if (fileTemplate[ii] == '%' && fileTemplate[ii + 1] != '%')
            nPrintfTokens++;
    }

    if (nPrintfTokens > 1) {
        isBinary = true;
        isParalellFormat = true;
    }

    if (!isParalellFormat && nPrintfTokens != 1) {
        sendError("Nek: The filetemplate tag must receive only one printf token for serial Nek files.");
    } else if (isParalellFormat && (nPrintfTokens < 2 || nPrintfTokens > 3)) {
        sendError("Nek: The filetemplate tag must receive either 2 or 3 printf tokens for parallel Nek files.");
    }
    int bufSize = fileTemplate.size();
    int len;
    string s;
    do
    {
        bufSize += 64;
        char *outFileName = new char[bufSize];
        if (!isParalellFormat)
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
}
