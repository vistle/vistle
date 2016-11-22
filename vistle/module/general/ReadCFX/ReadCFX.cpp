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
#include <cfxExport.h>  // linked but still qtcreator doesn't find it
#include <getargs.h>    // linked but still qtcreator doesn't find it
#include <iostream>
//#include <unistd.h>

//#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/cstdint.hpp>

#include "ReadCFX.h"

namespace bf = boost::filesystem;

MODULE_MAIN(ReadCFX)

using namespace vistle;

int checkFile(const char *filename)
{
    const int MIN_FILE_SIZE = 1024; //1024 minimal size for .res files [in Byte]

    const int MACIC_LEN = 5; // "magic" at the start
    const char *magic = "*INFO";
    char magicBuf[MACIC_LEN];

    boost::uintmax_t fileSize;
    boost::system::error_code ec;

    FILE *fi = fopen(filename, "r");

    if (!fi)
        return errno;
    else {
        fileSize = bf::file_size(filename, ec);
        if (ec)
            std::cout << "error code: " << ec << std::endl;
    }

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
   : Module("ReadCFX", shmname, name, moduleID)
{

    p_outPort1 = createOutputPort("GridOut0", "UnstructuredGrid: unstructured grid");
    p_outPort2 = createOutputPort("DataOut0", "Float: scalar data");
    p_outPort3 = createOutputPort("DataOut1", "Vec3: vector data");
    p_outPort4 = createOutputPort("GridOut1", "Polygons: region grid");
    p_outPort5 = createOutputPort("DataOut2", "Float: region scalar data");
    p_outPort6 = createOutputPort("GridOut2", "Polygons: boundary grid");
    p_outPort7 = createOutputPort("DataOut3", "Float: boundary scalar data");
    p_outPort8 = createOutputPort("DataOut4", "Vec3: boundary vector data");
    p_outPort9 = createOutputPort("GridOut3", "Points: particle points");
    p_outPort10 = createOutputPort("DataOut5", "Float: particle scalar data");
    p_outPort11 = createOutputPort("DataOut6", "Vec3: particle vector data");


    addStringParameter("filename", "name of file (%1%: block, %2%: timestep)", "/mnt/raid/home/hpcjwint/data/cfx/rohr/hlrs_002.res");
    //addStringParameter("filename", "name of file (%1%: block, %2%: timestep)", "/home/jwinterstein/data/cfx/rohr/hlrs_002.res");

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



bool ReadCFX::parameterChanged(const Parameter *p)
{
    std::string c = getStringParameter("filename");
    resultfileName = c.c_str();

    int checkValue = checkFile(resultfileName);
    std::cerr << "checkValue = " << checkValue << std::endl;

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
    }

    static char *resultName = NULL;
    if (resultName == NULL || strcmp(resultName, resultfileName) != 0) {
        resultName = new char[strlen(resultfileName) + 1];
        strcpy(resultName, resultfileName);

        sendInfo("Please wait...");

        if (nzones > 0)
            cfxExportDone();

        //nzones = cfxExportInit(resultName, NULL);
        nzones = cfxExportInit(resultName, counts);

        for(i=0;i<cfxCNT_SIZE;i++) {
            std::cerr << "counts[" << i << "] = " << counts[i] << std::endl;
        }

        std::cerr << "nzones: " << nzones << std::endl;
        std::cerr << "cfxCNT_SIZE: " << cfxCNT_SIZE << std::endl;

        for(i=0;i<nzones;i++) {
            std::cerr << "cfxExportZoneSet(): " << cfxExportZoneSet(i,NULL) << std::endl;
            std::cerr << "cfxExportZoneGet(): " << cfxExportZoneGet() << std::endl;
            std::cerr << "cfxExportTimestepCount(): " << cfxExportTimestepCount() << std::endl;
            std::cerr << "cfxExportTimestepNumGet(1): " << cfxExportTimestepNumGet(1) << std::endl;
            std::cerr << "cfxExportTimestepTimeGet(1): " << cfxExportTimestepTimeGet(1) << std::endl;
            std::cerr << "cfxExportNodeCount(): " << cfxExportNodeCount() << std::endl;
            std::cerr << "cfxExportElementCount(): " << cfxExportElementCount() << std::endl;
            std::cerr << "cfxExportZoneGet(): " << cfxExportZoneGet() << std::endl;
            std::cerr << "cfxExportZoneCount(): " << cfxExportZoneCount() << std::endl;
            std::cerr << "cfxExportRegionCount(): " << cfxExportRegionCount() << std::endl;
            std::cerr << "cfxExportVolumeCount(): " << cfxExportVolumeCount() << std::endl;
            std::cerr << "cfxExportBoundaryCount(): " << cfxExportBoundaryCount() << std::endl << std::endl;
        }




        timeStepNum = cfxExportTimestepNumGet(1);
        if (timeStepNum < 0) {
            sendInfo("no timesteps");
        }

        iteration = cfxExportTimestepNumGet(1);
        if (cfxExportTimestepSet(iteration) < 0)
        {
            sendInfo("Invalid timestep %d", iteration);
        }

        sendInfo("Found %d zones", nzones);

        // @@@ cfxExportDone();
        sendInfo("The initialisation was successfully done");

    }



 /* COVISE Initialisierung

        if (!in_map_loading)
        {

            p_zone->updateValue(nzones + 1, ZoneChoiceVal, p_zone->getValue());

            p_scalar->updateValue(nscalars + 1, ScalChoiceVal, p_scalar->getValue());
            p_vector->updateValue(nvectors + 1, VectChoiceVal, p_vector->getValue());

            p_boundScalar->updateValue(nscalars + 1, ScalChoiceVal, p_boundScalar->getValue());
            p_boundVector->updateValue(nvectors + 1, VectChoiceVal, p_boundVector->getValue());

            p_particleScalar->updateValue(npscalars + 1, pScalChoiceVal, p_particleScalar->getValue());
            p_particleVector->updateValue(npvectors + 1, pVectChoiceVal, p_particleVector->getValue());
        }
    }
    else
    {
        resultName = new char[strlen(resultfileName) + 1];
        strcpy(resultName, resultfileName);
    }
}
else if (strcmp(p_zone->getName(), paramName) == 0 && !in_map_loading)
{
    if (nzones > 0)
    {
        p_zone->updateValue(nzones + 1, ZoneChoiceVal, p_zone->getValue());
        get_choice_fields(p_zone->getValue());
        p_scalar->updateValue(nscalars + 1, ScalChoiceVal, p_scalar->getValue());
        p_vector->updateValue(nvectors + 1, VectChoiceVal, p_vector->getValue());
        p_boundScalar->updateValue(nscalars + 1, ScalChoiceVal, p_boundScalar->getValue());
        p_boundVector->updateValue(nvectors + 1, VectChoiceVal, p_boundVector->getValue());
        p_particleScalar->updateValue(npscalars + 1, pScalChoiceVal, p_particleScalar->getValue());
        p_particleVector->updateValue(npvectors + 1, pVectChoiceVal, p_particleVector->getValue());
    }
}
else if (strcmp(p_particleType->getName(), paramName) == 0 && !in_map_loading)
{
    setParticleVarChoice(p_particleType->getValue());
}
else if (strcmp(p_timesteps->getName(), paramName) == 0)
{
    if (nzones > 0)
    {
        p_zone->updateValue(nzones + 1, ZoneChoiceVal, p_zone->getValue());
        //get_choice_fields(p_zone->getValue());
        p_scalar->updateValue(nscalars + 1, ScalChoiceVal, p_scalar->getValue());
        p_vector->updateValue(nvectors + 1, VectChoiceVal, p_vector->getValue());
        p_boundScalar->updateValue(nscalars + 1, ScalChoiceVal, p_boundScalar->getValue());
        p_boundVector->updateValue(nvectors + 1, VectChoiceVal, p_boundVector->getValue());
        p_particleScalar->updateValue(npscalars + 1, pScalChoiceVal, p_particleScalar->getValue());
        p_particleVector->updateValue(npvectors + 1, pVectChoiceVal, p_particleVector->getValue());
    }
}
*/
   return Module::parameterChanged(p);
}

bool ReadCFX::compute() {

    std::cerr << "Compute Start. \n";


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
