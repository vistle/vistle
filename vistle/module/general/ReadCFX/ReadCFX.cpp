#include <string>
#include <sstream>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/triangles.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/normals.h>
#include <core/unstr.h>

#include <util/coRestraint.h>

// Includes for the CFX application programming interface (API)
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cfxExport.h>  // linked but still qtcreator doesn't find it
#include <getargs.h>    // linked but still qtcreator doesn't find it
#include <iostream>
//#include <unistd.h>

//#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/cstdint.hpp>

#include "ReadCFX.h"

namespace bf = boost::filesystem;

MODULE_MAIN(ReadCFX)

using namespace vistle;


ReadCFX::ReadCFX(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadCFX", shmname, name, moduleID) {

    // file browser parameter
//    m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/mnt/raid/home/hpcjwint/data/cfx/rohr/hlrs_002.res", Parameter::Directory);
    m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/data/eckerle/HLRS_Visualisierung_01122016/Betriebspunkt_250_3000/Configuration3_001.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/home/jwinterstein/data/cfx/rohr/hlrs_002.res", Parameter::Directory);

    // time parameters
    m_starttime = addFloatParameter("starttime", "start reading at the first step after this time", 0.);
    setParameterMinimum<Float>(m_starttime, 0.);
    m_stoptime = addFloatParameter("stoptime", "stop reading at the last step before this time",
          std::numeric_limits<double>::max());
    setParameterMinimum<Float>(m_stoptime, 0.);
    m_timeskip = addIntParameter("timeskip", "skip this many timesteps after reading one", 0);
    setParameterMinimum<Integer>(m_timeskip, 0);

    //zone selection
    m_zoneSelection = addStringParameter("zones","select zone numbers e.g. 1,4,6-10","all");

    // mesh ports
    m_gridOut = createOutputPort("grid_out1");

    // data ports and data choice parameters
    for (int i=0; i<NumPorts; ++i) {
        {// data ports
            std::stringstream s;
            s << "data_out" << i;
            m_volumeDataOut.push_back(createOutputPort(s.str()));
        }
        {// data choice parameters
            std::stringstream s;
            s << "Data" << i;
            auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back("(NONE)");
            setParameterChoices(p, choices);
            m_fieldOut.push_back(p);
        }
    }
    m_readBoundary = addIntParameter("read_boundary", "load the boundary?", 0, Parameter::Boolean);
    //m_boundaryPatchesAsVariants = addIntParameter("patches_as_variants", "create sub-objects with variant attribute for boundary patches", 1, Parameter::Boolean);

    // 2d data ports and 2d data choice parameters
    for (int i=0; i<NumBoundaryPorts; ++i) {
       {// 2d data ports
          std::stringstream s;
          s << "data_2d_out" << i;
          m_boundaryDataOut.push_back(createOutputPort(s.str()));
       }
       {// 2d data choice parameters
          std::stringstream s;
          s << "Data2d" << i;
          auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
          std::vector<std::string> choices;
          choices.push_back("(NONE)");
          setParameterChoices(p, choices);
          m_boundaryOut.push_back(p);
       }
    }
    //m_buildGhostcellsParam = addIntParameter("build_ghostcells", "whether to build ghost cells", 1, Parameter::Boolean);ll


   /*addIntParameter("indexed_geometry", "create indexed geometry?", 0, Parameter::Boolean);
   addIntParameter("triangulate", "only create triangles", 0, Parameter::Boolean);

   addIntParameter("first_block", "number of first block", 0);
   addIntParameter("last_block", "number of last block", 0);
*/
//   addIntParameter("first_step", "number of first timestep", 0);
//   addIntParameter("last_step", "number of last timestep", 0);
//   addIntParameter("step", "increment", 1);
}

ReadCFX::~ReadCFX() {

}

CaseInfo::CaseInfo()
    : m_valid(false) {

}

bool CaseInfo::checkFile(const char *filename) {
    const int MIN_FILE_SIZE = 1024; // minimal size for .res files [in Byte]

    const int MACIC_LEN = 5; // "magic" at the start
    const char *magic = "*INFO";
    char magicBuf[MACIC_LEN];

    boost::uintmax_t fileSize;
    boost::system::error_code ec;

    FILE *fi = fopen(filename, "r");

    if (!fi) {
        std::cout << filename << strerror(errno) << std::endl;
        return false;
    }
    else {
        fileSize = bf::file_size(filename, ec);
        if (ec)
            std::cout << "error code: " << ec << std::endl;
    }

    if (fileSize < MIN_FILE_SIZE) {
        std::cout << filename << "too small to be a real result file" << std::endl;
        std::cout << fileSize << "filesize" << std::endl;
        return false;
    }

    size_t iret = fread(magicBuf, 1, MACIC_LEN, fi);
    if (iret != MACIC_LEN) {
        std::cout << "checkFile :: error reading MACIC_LEN " << std::endl;
        return false;
    }

    if (strncasecmp(magicBuf, magic, MACIC_LEN) != 0) {
        std::cout << filename << "does not start with '*INFO'" << std::endl;
        return false;
    }

    fclose(fi);
    return true;
}

void CaseInfo::getFieldList() {

    int usr_level = 0;
    int dimension, corrected_boundary_node;
    int length;

    index_t nvars = cfxExportVariableCount(usr_level);
    //std::cerr << "nvars = " << nvars << std::endl;

    m_boundary_param.clear();
    m_boundary_param.push_back("(NONE)");
    m_field_param.clear();
    m_field_param.push_back("(NONE)");

    for(index_t varnum=1;varnum<=nvars;varnum++) {   //starts from 1 because cfxExportVariableName(varnum,1) only returnes values from 1 and higher
        if(cfxExportVariableSize(varnum,&dimension,&length,&corrected_boundary_node)) { //cfxExportVariableSize returns 1 if successful or 0 if the variable is out of range
            if(length == 1) {
                m_boundary_param.push_back(cfxExportVariableName(varnum,1)); //0 is short form and 1 is long form of the variable name
                //std::cerr << "cfxExportVariableName("<< varnum << ",1) = " << cfxExportVariableName(varnum,1) << std::endl;
            }
            else {
                m_field_param.push_back(cfxExportVariableName(varnum,1)); //0 is short form and 1 is long form of the variable name
            }
        }
    }
}

/*int ReadFOAM::rankForBlock(int processor) const {

   if (m_case.numblocks == 0)
      return 0;

   if (processor == -1)
      return -1;

   return processor % size();
}*/

/*int ReadCFX::rankForBlock(int block) const {

    const int numBlocks = m_lastBlock-m_firstBlock+1;
    if (numBlocks == 0)
        return 0;

    if (block == -1)
        return -1;

    return block % size();
}*/

bool ReadCFX::parameterChanged(const Parameter *p) {
    auto sp = dynamic_cast<const StringParameter *>(p);
    if (sp == m_resultfiledir) {
        std::string c = sp->getValue();
        const char *resultfiledir;
        resultfiledir = c.c_str();
        m_case.m_valid = m_case.checkFile(resultfiledir);
        if (!m_case.m_valid) {
           std::cerr << resultfiledir << " is not a valid CFX .res file" << std::endl;
           return false;
        }
        else {
            sendInfo("Please wait...");

            if (m_nzones > 0) {
                cfxExportDone();
            }
            char *resultfileName = strdup(resultfiledir);
            m_nzones = cfxExportInit(resultfileName, counts);
                m_nnodes = counts[cfxCNT_NODE];
                m_nelems = counts[cfxCNT_ELEMENT];
                //m_nregions = counts[cfxCNT_REGION];
                m_nvolumes = counts[cfxCNT_VOLUME];
                //m_nvars = counts[cfxCNT_VARIABLE];
            if (m_nzones < 0) {
                cfxExportDone();
                sendError("cfxExportInit could not open %s", resultfileName);
                return false;
            }

            int timeStepNum = cfxExportTimestepNumGet(1);
            if (timeStepNum < 0) {
                sendInfo("no timesteps");
            }

//            std::cerr << "cfxExportTimestepCount()" << cfxExportTimestepCount() << std::endl;
//            std::cerr << "cfxExportTimestepSet(timeStepNum)" << cfxExportTimestepSet(timeStepNum) << std::endl;
//            std::cerr << "cfxExportTimestepNumGet(1)" << cfxExportTimestepNumGet(1) << std::endl;

            cfxExportTimestepSet(timeStepNum);
            if (cfxExportTimestepSet(timeStepNum) < 0) {
                sendInfo("Invalid timestep %d", timeStepNum);
            }

            //fill choice parameter
            m_case.getFieldList();
            for (auto out: m_fieldOut) {
               setParameterChoices(out, m_case.m_field_param);
            }
            for (auto out: m_boundaryOut) {
                setParameterChoices(out, m_case.m_boundary_param);
            }
//            std::vector<std::string>::const_iterator it1;
//            for(it1=m_case.m_field_param.begin();it1!=m_case.m_field_param.end();++it1) {
//                std::cerr << "field_choices = " << *it1 << std::endl;
//            }
//            for(it1=m_case.m_boundary_param.begin();it1!=m_case.m_boundary_param.end();++it1) {
//                std::cerr << "field_choices = " << *it1 << std::endl;
//            }

            //print out zone names
            sendInfo("Found %d zones", m_nzones);
            for(index_t i=1;i<=m_nzones;i++) {
                cfxExportZoneSet(i,NULL);
                sendInfo("name of zone no. %d: %s \n",i,cfxExportZoneName(i));
                cfxExportZoneFree();
            }

            free(resultfileName);
            sendInfo("The initialisation was successfully done");
        }
    }

    return Module::parameterChanged(p);
}

bool ReadCFX::loadGrid(int volumeNr) {

    if(cfxExportZoneSet(m_selectedVolumes[volumeNr].zoneFlag,counts) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    std::cerr << "m_selectedVolumes[volumeNr].volumeID = " << m_selectedVolumes[volumeNr].volumeID << "; m_selectedVolumes[volumeNr].zoneFlag = " << m_selectedVolumes[volumeNr].zoneFlag << std::endl;

    index_t nnodesInVolume, nelmsInVolume;
    nnodesInVolume = cfxExportVolumeSize(m_selectedVolumes[volumeNr].volumeID,cfxVOL_NODES);
    nelmsInVolume = cfxExportVolumeSize(m_selectedVolumes[volumeNr].volumeID,cfxVOL_ELEMS);

    UnstructuredGrid::ptr grid(new UnstructuredGrid(nelmsInVolume, 8*nelmsInVolume, nnodesInVolume)); //initialized with number of elements, number of connectivities, number of coordinates

    //load nodes into unstructured grid
    boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);
    //double *x_coord = new double, *y_coord = new double, *z_coord = new double;
    //boost::shared_ptr<std::int32_t> nodeListOfVolume(new int);
    int *nodeListOfVolume = new int;

    //boost::shared_ptr<std::double_t> ptrOnXdata(new double);
    auto ptrOnXdata = grid->x().data();
    auto ptrOnYdata = grid->y().data();
    auto ptrOnZdata = grid->z().data();

    nodeListOfVolume = cfxExportVolumeList(m_selectedVolumes[volumeNr].volumeID,cfxVOL_NODES); //query the nodes that define the volume
    std::cerr << "nnodesInVolume = " << nnodesInVolume << std::endl;
    for(index_t i=1;i<=nnodesInVolume;++i) {
        if(!cfxExportNodeGet(nodeListOfVolume[i],x_coord.get(),y_coord.get(),z_coord.get())) {  //get access to coordinates: [IN] nodeid [OUT] x,y,z
            std::cerr << "error while reading nodes out of .res file: nodeid is out of range" << std::endl;
        }

        if(i<40) {
            std::cerr << "x,y,z (" << i << ") = " << *x_coord.get() << ", " << *y_coord.get() << ", " << *z_coord.get() << std::endl;
            std::cerr << "nodeListOfVolume[" <<i<< "] = " << nodeListOfVolume[i] << std::endl;
        }
        ptrOnXdata[i] = *x_coord.get();
        ptrOnYdata[i] = *y_coord.get();
        ptrOnZdata[i] = *z_coord.get();
    }


    //Test, ob Einlesen funktioniert hat
    //        std::cerr << "m_nnodes = " << m_nnodes << std::endl;
    //        std::cerr << "grid->getNumCoords()" << grid->getNumCoords() << std::endl;
    for(int i=0;i<20;++i) {
            std::cerr << "x,y,z (" << i << ") = " << grid->x().at(i) << ", " << grid->y().at(i) << ", " << grid->z().at(i) << std::endl;
    }
    //        std::cerr << "x,y,z (10)" << grid->x().at(10) << ", " << grid->y().at(10) << ", " << grid->z().at(10) << std::endl;
    //        std::cerr << "x,y,z (m_nnodes-1)" << grid->x().at(m_nnodes-1) << ", " << grid->y().at(m_nnodes-1) << ", " << grid->z().at(m_nnodes-1) << std::endl;


    cfxExportNodeFree();
    cfxExportVolumeFree(m_selectedVolumes[volumeNr].volumeID);
    delete[] nodeListOfVolume;




/*    //load element types, element list and connectivity list into unstructured grid
    int elemListCounter=0;
    boost::shared_ptr<std::int32_t> nodelist(new int[8]), elemtype(new int);

    m_nelems = cfxExportElementCount();
    elmList = cfxExportElementList(); //load elements into array with structs, each struct containing type and array with nodeid's which belong to the element

    for(index_t elemid=1;elemid<=m_nelems;++elemid) {
        if(!cfxExportElementGet(elemid,elemtype.get(),nodelist.get())) {
            std::cerr << "error while reading elements out of .res file: elemid is out of range" << std::endl;
        }
        switch(*elemtype.get()) {
            case 4: {
                elemListCounter += 4;
                grid->tl().push_back(UnstructuredGrid::TETRAHEDRON);
                grid->el().push_back(elemListCounter);
                for (int nodelist_counter=0;nodelist_counter<4;++nodelist_counter) {
                    grid->cl().push_back(nodelist.get()[nodelist_counter]-1);
                }
                break;
            }
            case 5: {
                elemListCounter += 5;
                grid->tl().push_back(UnstructuredGrid::PYRAMID);
                grid->el().push_back(elemListCounter);
                for (int nodelist_counter=0;nodelist_counter<5;++nodelist_counter) {
                    grid->cl().push_back(nodelist.get()[nodelist_counter]-1);
                }
                break;
            }
            case 6: {
                elemListCounter += 6;
                grid->tl().push_back(UnstructuredGrid::PRISM);
                grid->el().push_back(elemListCounter);
                for (int nodelist_counter=0;nodelist_counter<6;++nodelist_counter) {
                    grid->cl().push_back(nodelist.get()[nodelist_counter]-1);
                }
                break;
            }
            case 8: {
                elemListCounter += 8;
                grid->tl().push_back(UnstructuredGrid::HEXAHEDRON);
                grid->el().push_back(elemListCounter);
                //std::cerr << "elemid = " << elemid << "; elemtype = " << *elemtype.get() <<  std::endl;
                for (int nodelist_counter=0;nodelist_counter<8;++nodelist_counter) {
                    grid->cl().push_back(nodelist.get()[nodelist_counter]-1);
                    //std::cerr << "nodelist(" << nodelist_counter << ") = " << nodelist.get()[nodelist_counter]-1 << std::endl;
                }
                break;
            }
        }
    }

    //Test, ob Einlesen funktioniert hat
//        std::cerr << "tets = " << counts[cfxCNT_TET] << "; pyramids = " << counts[cfxCNT_PYR] << "; prism = " << counts[cfxCNT_WDG] << "; hexaeder = " << counts[cfxCNT_HEX] << std::endl;
//        std::cerr <<"no. elems = " << m_nelems << std::endl;
//        std::cerr <<"grid->getNumElements" << grid->getNumElements() << std::endl;
//        for(index_t i = 0;i<19;++i) {
//            std::cerr << "tl(" << i << ") = " << grid->tl().at(i) << std::endl;
//            std::cerr << "el(" << i << ") = " << grid->el().at(i) << std::endl;
//            for(index_t j = 0;j<8;++j) {
//                std::cerr << "cl(" << i*8+j << ") = " << grid->cl().at(i*8+j) << std::endl;
//            }
//        }

    cfxExportElementFree();
*/
    return true;
}

bool ReadCFX::loadField(int volumeNr) {


//    std::cerr << "m_boundaryOut[0] = " << m_boundaryOut[0]->getValue() << std::endl;
//    std::cerr << "m_boundaryOut[1] = " << m_boundaryOut[1]->getValue() << std::endl;
//    std::cerr << "m_boundaryOut[2] = " << m_boundaryOut[2]->getValue() << std::endl;


    return true;
}

int ReadCFX::collectVolumes() {
    // read zone selection; m_zonesSelected contains a bool array of which zones are selected
        //m_zonesSelected(zone) = 1 Zone ist selektiert
        //m_zonesSelected(zone) = 0 Zone ist nicht selektiert
        //group = -1 alle Zonen sind selektiert
    m_zonesSelected.clear();
    m_zonesSelected.add(m_zoneSelection->getValue());
    ssize_t val = m_zonesSelected.getNumGroups(), group;
    m_zonesSelected.get(val,group);

    int k=0;
    m_selectedVolumes.reserve(m_nvolumes);

    for(index_t i=1;i<=m_nzones;++i) {
        if(m_zonesSelected(i)) {
            if(cfxExportZoneSet(i,NULL)<0) {
                std::cerr << "invalid zone number" << std::endl;
            }
            int nvolumes = cfxExportVolumeCount();
            for(int j=1;j<nvolumes+1;++j) {
                //std::cerr << "volumeName = " << cfxExportVolumeName((int) j) << std::endl;
                m_selectedVolumes[k]=VolumeIdWithZoneFlag(j,i);
                k++;
            }
        }
    }

    //zum Testen
//    for(index_t i=0;i<k;++i) {
//        std::cerr << "m_selectedVolumes[" << i << "].volumeID = " << m_selectedVolumes[i].volumeID << " m_selectedVolumes.zoneFlag" << m_selectedVolumes[i].zoneFlag << std::endl;
//    }

    return k;
}

bool ReadCFX::compute() {

    std::cerr << "Compute Start. \n";

    int numbSelVolumes = collectVolumes();
    //for Schleife über alle Volumes, für jedes Volume ein unstructured grid
    sendInfo("Schleife über Volumes Start");
    for(int i=0;i<numbSelVolumes;++i) {
        loadGrid(i);
        //loadField(i);
    }
    sendInfo("Schleife über Volumes End");

//    int *nodesinVolume;
//    for(index_t i=0;i<=m_nzones;++i) {
//        cfxExportZoneSet(i,counts);
//        std::cerr << "Zone = " << i << "; Nodes = " << counts[cfxCNT_NODE] << "; volumes = " << counts[cfxCNT_VOLUME] << std::endl << std::endl;

//        for(int j=1;j<=counts[cfxCNT_VOLUME];++j) {
//            std::cerr << "Size of nodes in volume = " << cfxExportVolumeSize(j,cfxVOL_NODES) << std::endl;
//            std::cerr << "Name of region = " << cfxExportVolumeName(j) << std::endl;
//           // nodesinVolume = cfxExportVolumeList(j,cfxVOL_NODES);
//            for(int k=0;k<3;k++) {
//                std::cerr << "nodes = " << nodesinVolume[k] << std::endl;
//            }
//            cfxExportVolumeFree(j);
//        }
//    }








//    for(t = t1; t <= t2; t++) {
//    ts = cfxExportTimestepNumGet(t);
//    if(cfxExportTimestepSet(ts) < 0) {
//    continue;
//    }

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

       }
       if (loaded)
           ++timeCounter;
   }*/

    cfxExportDone();

   return true;
}
