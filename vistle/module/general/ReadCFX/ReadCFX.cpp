#include <string>

#include <boost/format.hpp>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/triangles.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/normals.h>

// Includes for the CFX application programming interface (API)
#include <cstdio>
#include <cstring>
#include <cstdlib>
// #include <io.h>         // unclear, what this library is doing
#include <cfxExport.h>  // linked but still qtcreator doesn't find it
#include <getargs.h>    // linked but still qtcreator doesn't find it
#include <iostream>

#include "ReadCFX.h"

MODULE_MAIN(ReadCFX)

using namespace vistle;

int checkFile(const char *filename)
{
    const int MIN_FILE_SIZE = 1024; // minimal size for .res files

    const int MACIC_LEN = 5; // "magic" at the start
    const char *magic = "*INFO";
    char magicBuf[MACIC_LEN];

    struct stat statRec;
    off_t fileSize;
#ifdef WIN32
    HANDLE hFile = CreateFile(filename, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "could not open file %s\n", filename);
        return -1; // error condition, could call GetLastError to find out more
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size))
    {
        CloseHandle(hFile);
        fprintf(stderr, "could not get filesize for %s\n", filename);
        return -1; // error condition, could call GetLastError to find out more
    }
    fileSize = size.QuadPart;

    CloseHandle(hFile);
#else
    if (stat(filename, &statRec) < 0)
        return errno;
    fileSize = statRec.st_size;
#endif

    FILE *fi = fopen(filename, "r");
    if (!fi)
        return errno;

    if (fileSize < MIN_FILE_SIZE)
        return -1;

    size_t iret = fread(magicBuf, 1, MACIC_LEN, fi);
    if (iret != MACIC_LEN)
        std::cerr << "checkFile :: error reading MACIC_LEN " << std::endl;

    if (strncasecmp(magicBuf, magic, MACIC_LEN) != 0)
        return -2;

    fclose(fi);

    return 0;
}

ReadCFX::ReadCFX(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadModel", shmname, name, moduleID)
{

  // createOutputPort("grid_out");
   addStringParameter("filename", "name of file (%1%: block, %2%: timestep)", "/home/jwinterstein/data/cfx/rohr/hlrs_002.res");

   /*addIntParameter("indexed_geometry", "create indexed geometry?", 0, Parameter::Boolean);
   addIntParameter("triangulate", "only create triangles", 0, Parameter::Boolean);

   addIntParameter("first_block", "number of first block", 0);
   addIntParameter("last_block", "number of last block", 0);

   addIntParameter("first_step", "number of first timestep", 0);
   addIntParameter("last_step", "number of last timestep", 0);
   addIntParameter("step", "increment", 1);

   addIntParameter("ignore_errors", "ignore files that are not found", 0, Parameter::Boolean);
*/
}

ReadCFX::~ReadCFX() {

}

    //Open CFX result file and initialize Export API
    //int cfxExportInit (char *resfile, int counts[cfxCNT_SIZE])

/*int ReadCFX::rankForBlock(int block) const {

    const int numBlocks = m_lastBlock-m_firstBlock+1;
    if (numBlocks == 0)
        return 0;

    if (block == -1)
        return -1;

    return block % size();
}*/


Object::ptr ReadCFX::load(const std::string &name) {

    Object::ptr ret;

    return ret;
}

bool ReadCFX::compute() {

    std::cerr << "Compute Start. \n";

    std::string resultfileName = getStringParameter("filename");
    const char *c = resultfileName.c_str();

    int checkValue = checkFile(c);
    std::cerr << "checkValue = " << checkValue << std::endl;
/*
    if (checkValue != 0)
    {
        if (checkValue > 0)
            sendError("'%s':%s", resultfileName, strerror(checkValue));
        else
            switch (checkValue)
            {

            case -1:
                sendError("'%s': too small to be a real result file",
                          resultfileName);
                break;
            case -2:
                sendError("'%s':does not start with '*INFO'",
                          resultfileName);
            }
        return;
    }*/
 /*  m_firstBlock = getIntParameter("first_block");
   m_lastBlock = getIntParameter("last_block");
   m_firstStep = getIntParameter("first_step");
   m_lastStep = getIntParameter("last_step");
   m_step = getIntParameter("step");
   if ((m_lastStep-m_firstStep)*m_step < 0)
       m_step *= -1;
   bool reverse = false;
   int step = m_step, first = m_firstStep, last = m_lastStep;
   if (step < 0) {
       reverse = true;
       step *= -1;
       first *= -1;
       last *= -1;
   }

   std::string filename = getStringParameter("filename");

   int timeCounter = 0;
   for (int t=first; t<=last; t += step) {
       int timestep = reverse ? -t : t;
       bool loaded = false;
       for (int b=m_firstBlock; b<=m_lastBlock; ++b) {
           if (rankForBlock(b) == rank()) {
               std::string f;
               try {
                   using namespace boost::io;
                   boost::format fmter(filename);
                   fmter.exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit));
                   fmter % b;
                   fmter % timestep;
                   f = fmter.str();
               } catch (boost::io::bad_format_string except) {
                   sendError("bad format string in filename");
                   return true;
               }
               auto obj = load(f);
               if (!obj) {
                   if (!getIntParameter("ignore_errors")) {
                       sendError("failed to load %s", f.c_str());
                       return true;
                   }
               } else {
                   obj->setBlock(b);
                   obj->setTimestep(timeCounter);
                   loaded = true;
                   addObject("grid_out", obj);
               }
           }
       }
       if (loaded)
           ++timeCounter;
   }*/

   return true;
}
