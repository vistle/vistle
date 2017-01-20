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
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/mnt/raid/home/hpcjwint/data/cfx/rohr/hlrs_002.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/data/eckerle/HLRS_Visualisierung_01122016/Betriebspunkt_250_3000/Configuration3_001.res", Parameter::Directory);
    m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/home/jwinterstein/data/cfx/rohr/hlrs_002.res", Parameter::Directory);

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
    //m_readBoundary = addIntParameter("read_boundary", "load the boundary?", 0, Parameter::Boolean);
    //m_boundaryPatchesAsVariants = addIntParameter("patches_as_variants", "create sub-objects with variant attribute for boundary patches", 1, Parameter::Boolean);
    m_boundarySelection = addStringParameter("boundaries","select boundary numbers e.g. 1,4,6-10","all");

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

void CaseInfo::parseResultfile() {
    cfxExportZoneSet(0,NULL);
    int dimension, corrected_boundary_node, length;

    m_nvars = cfxExportVariableCount(ReadCFX::usr_level);
    m_nbounds = cfxExportBoundaryCount();
    m_ParamDimension[0] = 0;
    m_allParam.clear();
    m_allParam.insert(bm_type::value_type(0,"(NONE)"));

    for(index_t varnum=1;varnum<=m_nvars;varnum++) {   //starts from 1 because cfxExportVariableName(varnum,ReadCFX::alias) only returnes values from 1 and higher
        m_allParam.insert(bm_type::value_type(varnum,cfxExportVariableName(varnum,ReadCFX::alias)));
        if(cfxExportVariableSize(varnum,&dimension,&length,&corrected_boundary_node)) { //cfxExportVariableSize returns 1 if successful or 0 if the variable is out of range
            if(dimension == 1) {
                m_ParamDimension[varnum] = 1;
            }
            else if(dimension == 3) {
                m_ParamDimension[varnum] = 3;
            }
        }
    }
    for(index_t boundnum=1;boundnum<=m_nbounds;boundnum++) {
        m_allBoundaries.insert(bm_type::value_type(boundnum,cfxExportBoundaryName(boundnum)));
    }
    cfxExportZoneFree();
    return;
}

void CaseInfo::getFieldList() {
    cfxExportZoneSet(0,NULL);
    int dimension, corrected_boundary_node, length;

    m_boundary_param.clear();
    m_boundary_param.push_back("(NONE)");
    m_field_param.clear();
    m_field_param.push_back("(NONE)");


    for(index_t varnum=1;varnum<=m_nvars;varnum++) {   //starts from 1 because cfxExportVariableName(varnum,ReadCFX::alias) only returnes values from 1 and higher
        if(cfxExportVariableSize(varnum,&dimension,&length,&corrected_boundary_node)) { //cfxExportVariableSize returns 1 if successful or 0 if the variable is out of range
            if(length != 1) {
                m_field_param.push_back(m_allParam.left.at(varnum));
            }
            m_boundary_param.push_back(m_allParam.left.at(varnum));
        }
    }
    cfxExportZoneFree();
    return;
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
                ExportDone = true;
            }
            char *resultfileName = strdup(resultfiledir);
            m_nzones = cfxExportInit(resultfileName, counts);
                //m_nnodes = counts[cfxCNT_NODE];
                //m_nelems = counts[cfxCNT_ELEMENT];
                //m_nregions = counts[cfxCNT_REGION];
                m_nvolumes = counts[cfxCNT_VOLUME];
                //m_nvars = counts[cfxCNT_VARIABLE];
                ExportDone = false;
            if (m_nzones < 0) {
                cfxExportDone();
                sendError("cfxExportInit could not open %s", resultfileName);
                ExportDone = true;
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
            m_case.parseResultfile();
            m_case.getFieldList();
            for (auto out: m_fieldOut) {
               setParameterChoices(out, m_case.m_field_param);
            }
            for (auto out: m_boundaryOut) {
                setParameterChoices(out, m_case.m_boundary_param);
            }

            //print out zone names
            sendInfo("Found %d zones", m_nzones);
            for(index_t i=1;i<=m_nzones;i++) {
                cfxExportZoneSet(i,NULL);
                sendInfo("zone no. %d: %s",i,cfxExportZoneName(i));
                cfxExportZoneFree();
            }
            //print out boundary names
            cfxExportZoneSet(0,NULL);
            m_nboundaries = cfxExportBoundaryCount();
            sendInfo("Found %d boundaries", m_nboundaries);
            for(index_t i=1;i<=m_nboundaries;++i) {
                sendInfo("boundary no. %d: %s",i,cfxExportBoundaryName(i));
            }
            cfxExportZoneFree();

            free(resultfileName);
            sendInfo("The initialisation was successfully done");
        }
    }

    return Module::parameterChanged(p);
}

UnstructuredGrid::ptr ReadCFX::loadGrid(int volumeNr) {

    if(cfxExportZoneSet(m_volumesSelected[volumeNr].zoneFlag,counts) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    //std::cerr << "m_volumesSelected[volumeNr].ID = " << m_volumesSelected[volumeNr].ID << "; m_volumesSelected[volumeNr].zoneFlag = " << m_volumesSelected[volumeNr].zoneFlag << std::endl;

    index_t nnodesInVolume, nelmsInVolume, nconnectivities;
    nnodesInVolume = cfxExportVolumeSize(m_volumesSelected[volumeNr].ID,cfxVOL_NODES);
    nelmsInVolume = cfxExportVolumeSize(m_volumesSelected[volumeNr].ID,cfxVOL_ELEMS);
        //std::cerr << "nodesInVolume = " << nnodesInVolume << std::endl;
        //std::cerr << "nelmsInVolume = " << nelmsInVolume << std::endl;
        //std::cerr << "tets = " << counts[cfxCNT_TET] << ", " << "pyramid = " << counts[cfxCNT_PYR] << ", "<< "prism = " << counts[cfxCNT_WDG] << ", "<< "hex = " << counts[cfxCNT_HEX] << std::endl;
    nconnectivities = 4*counts[cfxCNT_TET]+5*counts[cfxCNT_PYR]+6*counts[cfxCNT_WDG]+8*counts[cfxCNT_HEX];

    UnstructuredGrid::ptr grid(new UnstructuredGrid(nelmsInVolume, nconnectivities, nnodesInVolume)); //initialized with number of elements, number of connectivities, number of coordinates

    //load coords into unstructured grid
    boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);

    auto ptrOnXcoords = grid->x().data();
    auto ptrOnYcoords = grid->y().data();
    auto ptrOnZcoords = grid->z().data();

    int *nodeListOfVolume = cfxExportVolumeList(m_volumesSelected[volumeNr].ID,cfxVOL_NODES); //query the nodes that define the volume
    for(index_t i=0;i<nnodesInVolume;++i) {
        if(!cfxExportNodeGet(nodeListOfVolume[i],x_coord.get(),y_coord.get(),z_coord.get())) {  //get access to coordinates: [IN] nodeid [OUT] x,y,z
            std::cerr << "error while reading nodes out of .res file: nodeid is out of range" << std::endl;
        }
        ptrOnXcoords[i] = *x_coord.get();
        ptrOnYcoords[i] = *y_coord.get();
        ptrOnZcoords[i] = *z_coord.get();
    }


    //Test, ob Einlesen funktioniert hat
    //        std::cerr << "m_nnodes = " << m_nnodes << std::endl;
    //        std::cerr << "grid->getNumCoords()" << grid->getNumCoords() << std::endl;
//    for(int i=0;i<20;++i) {
//            std::cerr << "x,y,z (" << i << ") = " << grid->x().at(i) << ", " << grid->y().at(i) << ", " << grid->z().at(i) << std::endl;
//    }
    //        std::cerr << "x,y,z (10)" << grid->x().at(10) << ", " << grid->y().at(10) << ", " << grid->z().at(10) << std::endl;
    //        std::cerr << "x,y,z (m_nnodes-1)" << grid->x().at(m_nnodes-1) << ", " << grid->y().at(m_nnodes-1) << ", " << grid->z().at(m_nnodes-1) << std::endl;


    cfxExportNodeFree();
    cfxExportVolumeFree(m_volumesSelected[volumeNr].ID);

    //load element types, element list and connectivity list into unstructured grid
    int elemListCounter=0;
    boost::shared_ptr<std::int32_t> nodesOfElm(new int[8]), elemtype(new int);
    auto ptrOnTl = grid->tl().data();
    auto ptrOnEl = grid->el().data();
    auto ptrOnCl = grid->cl().data();

    int *elmListOfVolume = cfxExportVolumeList(m_volumesSelected[volumeNr].ID,cfxVOL_ELEMS); //query the elements that define the volume

    for(index_t i=0;i<nelmsInVolume;++i) {
        if(!cfxExportElementGet(elmListOfVolume[i],elemtype.get(),nodesOfElm.get())) {
            std::cerr << "error while reading elements out of .res file: elemid is out of range" << std::endl;
        }
        switch(*elemtype.get()) {
            case 4: {
                ptrOnTl[i] = (UnstructuredGrid::TETRAHEDRON);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = nodesOfElm.get()[nodesOfElm_counter]-1; //-1 because cfx starts to count with 1; 1st node is in x().at(0)
                }
                elemListCounter += 4;
                break;
            }
            case 5: {
                ptrOnTl[i] = (UnstructuredGrid::PYRAMID);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<5;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = nodesOfElm.get()[nodesOfElm_counter]-1; //-1 because cfx starts to count with 1; 1st node is in x().at(0)
                }
                elemListCounter += 5;
                break;
            }
            case 6: {
                ptrOnTl[i] = (UnstructuredGrid::PRISM);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<6;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = nodesOfElm.get()[nodesOfElm_counter]-1; //-1 because cfx starts to count with 1; 1st node is in x().at(0)
                }
                elemListCounter += 6;
                break;
            }
            case 8: {
                ptrOnTl[i] = (UnstructuredGrid::HEXAHEDRON);
                ptrOnEl[i] = elemListCounter;
                //std::cerr << "elemid = " << elemid << "; elemtype = " << *elemtype.get() <<  std::endl;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<8;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = nodesOfElm.get()[nodesOfElm_counter]-1; //-1 because cfx starts to count with 1; 1st node is in x().at(0)
//                        std::cerr << "nodesOfElm(" << nodesOfElm_counter << ") = " << nodesOfElm.get()[nodesOfElm_counter]-1 << std::endl;
                }
                elemListCounter += 8;
                break;
            }
        }
    }

    //element after last element
    ptrOnEl[nelmsInVolume] = elemListCounter;
    ptrOnCl[elemListCounter] = 0;

    //Test, ob Einlesen funktioniert hat
//        std::cerr << "tets = " << counts[cfxCNT_TET] << "; pyramids = " << counts[cfxCNT_PYR] << "; prism = " << counts[cfxCNT_WDG] << "; hexaeder = " << counts[cfxCNT_HEX] << std::endl;
//        std::cerr <<"no. elems total = " << m_nelems << std::endl;
//        std::cerr <<"grid->getNumElements" << grid->getNumElements() << std::endl;
//        for(index_t i = nelmsInVolume-5;i<nelmsInVolume+1;++i) {
//            //std::cerr << "tl(" << i << ") = " << grid->tl().at(i) << std::endl;
//            std::cerr << "el(" << i << ") = " << grid->el().at(i) << std::endl;
//            for(index_t j = 0;j<1;++j) {
//                std::cerr << "cl(" << i*8+j << ") = " << grid->cl().at(i*8+j) << std::endl;
//            }
//        }

//    std::cerr << "grid->el(grid->getNumElements())" << grid->el().at(grid->getNumElements()) << std::endl;
//    std::cerr << "grid->getNumCorners()" << grid->getNumCorners() << std::endl;
//    std::cerr << "nconnectivities = " << nconnectivities << std::endl;
//    std::cerr << "elemListCounter = " << elemListCounter << std::endl;

//    for(int i=nelmsInVolume-10;i<=nelmsInVolume;++i) {
//        std::cerr << "ptrOnEl[" << i << "] = " << ptrOnEl[i] << std::endl;
//    }
//    for(int i=elemListCounter-10;i<=elemListCounter;++i) {
//        std::cerr << "ptrOnCl[" << i << "] = " << ptrOnCl[i] << std::endl;
//    }

    cfxExportElementFree();
    cfxExportVolumeFree(m_volumesSelected[volumeNr].ID);

    return grid;
}

Polygons::ptr ReadCFX::loadPolygon(int boundaryNr) {
    if(cfxExportZoneSet(m_boundariesSelected[boundaryNr].zoneFlag,counts) < 0) { //counts is a vector for statistics of the zone
        std::cerr << "invalid zone number" << std::endl;
    }
    std::cerr << "zone is set to = " << m_boundariesSelected[boundaryNr].zoneFlag << std::endl;

//    index_t nNodesInBoundary, nFacesInBoundary, nConnectInBoundary;
    index_t nNodesInBoundary, nFacesInBoundary;
    nNodesInBoundary = cfxExportBoundarySize(m_boundariesSelected[boundaryNr].ID,cfxREG_NODES);
    nFacesInBoundary = cfxExportBoundarySize(m_boundariesSelected[boundaryNr].ID,cfxREG_FACES);
//    nConnectInBoundary = 4*nFacesInBoundary; //passt nur, wenn alle Faces 4 Knoten haben. Das ist nicht immer der Fall!!
    //std::cerr << "tets = " << counts[cfxCNT_TET] << ", " << "pyramid = " << counts[cfxCNT_PYR] << ", "<< "prism = " << counts[cfxCNT_WDG] << ", "<< "hex = " << counts[cfxCNT_HEX] << std::endl;

//    Polygons::ptr polygon(new Polygons(nFacesInBoundary,nConnectInBoundary,nNodesInBoundary)); //initialize Polygon with numFaces, numCorners, numVertices
    Polygons::ptr polygon(new Polygons(nFacesInBoundary,0,nNodesInBoundary)); //initialize Polygon with numFaces, numCorners, numVertices

    //load coords into polygon
    boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);

    auto ptrOnXcoords = polygon->x().data();
    auto ptrOnYcoords = polygon->y().data();
    auto ptrOnZcoords = polygon->z().data();

    int *nodeListOfBoundary = cfxExportBoundaryList(m_boundariesSelected[boundaryNr].ID,cfxREG_NODES); //query the nodes that define the boundary
    for(index_t i=0;i<nNodesInBoundary;++i) {
        if(!cfxExportNodeGet(nodeListOfBoundary[i],x_coord.get(),y_coord.get(),z_coord.get())) {  //get access to coordinates: [IN] nodeid [OUT] x,y,z
            std::cerr << "error while reading nodes out of .res file: nodeid is out of range" << std::endl;
        }
        ptrOnXcoords[i] = *x_coord.get();
        ptrOnYcoords[i] = *y_coord.get();
        ptrOnZcoords[i] = *z_coord.get();
    }


    //Test, ob Einlesen funktioniert hat
    //    std::cerr << "nNodesInBoundary = " << nNodesInBoundary << std::endl;
    //    std::cerr << "polygon->getNumCoords()" << polygon->getNumCoords() << std::endl;
    //    for(int i=0;i<100;++i) {
    //        std::cerr << "x,y,z (" << i << ") = " << polygon->x().at(i) << ", " << polygon->y().at(i) << ", " << polygon->z().at(i) << std::endl;
    //    }

    cfxExportNodeFree();
    cfxExportBoundaryFree(m_boundariesSelected[boundaryNr].ID);

    //cfxExportZoneFree();

    //load face types, element list and connectivity list into polygon
    int elemListCounter=0;
    boost::shared_ptr<std::int32_t> nodesOfFace(new int[4]); // elemtype(new int);
    //auto ptrOnTl = polygon->tl().data();
    auto ptrOnEl = polygon->el().data();
    auto &ptrOnCl = polygon->cl();

    int *faceListofBoundary = cfxExportBoundaryList(m_boundariesSelected[boundaryNr].ID,cfxREG_FACES); //query the faces that define the boundary

    for(index_t i=0;i<nFacesInBoundary;++i) {
        int NumOfVerticesDefiningFace = cfxExportFaceNodes(faceListofBoundary[i],nodesOfFace.get());
        if(!NumOfVerticesDefiningFace) {
            std::cerr << "error while reading faces out of .res file: faceId is out of range" << std::endl;
        }

        switch(NumOfVerticesDefiningFace) {
        case 3: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<3;++nodesOfElm_counter) {
//                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodesOfFace.get()[nodesOfElm_counter]-1; //-1 because cfx starts to count with 1; 1st node is in x().at(0)
                ptrOnCl.push_back(nodesOfFace.get()[nodesOfElm_counter]-1); //-1 because cfx starts to count with 1; 1st node is in x().at(0)
            }
            elemListCounter += 3;
            break;
        }
        case 4: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
//                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodesOfFace.get()[nodesOfElm_counter]-1; //-1 because cfx starts to count with 1; 1st node is in x().at(0)
                ptrOnCl.push_back(nodesOfFace.get()[nodesOfElm_counter]-1); //-1 because cfx starts to count with 1; 1st node is in x().at(0)
            }
            elemListCounter += 4;
            break;
        }
        }

//        if(i<20) {
//            std::cerr << "faceListofBoundary[" << i << "] = " << faceListofBoundary[i] << std::endl;
//            for(int j=0;j<4;++j) {
//                std::cerr << "nodesOfFace[" << j << "] = " << nodesOfFace.get()[j] << std::endl;
//            }
//            std::cerr << "local face number[" << faceListofBoundary[i] << "] = " << cfxFACENUM(faceListofBoundary[i]) << std::endl;
//            std::cerr << "local element number[" << faceListofBoundary[i] << "] = " << cfxELEMNUM(faceListofBoundary[i]) << std::endl;
//            std::cerr << "size of nodesOfFace = " << NumOfVerticesDefiningFace << std::endl;
//        }
    }


    //element after last element in element list and connectivity list
    ptrOnEl[nFacesInBoundary] = elemListCounter;
//    ptrOnCl[elemListCounter] = 0;
    ptrOnCl.push_back(0);

    //Test, ob Einlesen funktioniert hat
//    std::cerr << "tets = " << counts[cfxCNT_TET] << "; pyramids = " << counts[cfxCNT_PYR] << "; prism = " << counts[cfxCNT_WDG] << "; hexaeder = " << counts[cfxCNT_HEX] << std::endl;
    std::cerr << "nodes = " << nNodesInBoundary << "; faces = " << nFacesInBoundary << "; connect = " << ptrOnCl.size() << std::endl;
    std::cerr <<"polygon->getNumVertices" << polygon->getNumVertices() << std::endl;
    std::cerr <<"polygon->getNumElements" << polygon->getNumElements() << std::endl;
    std::cerr <<"polygon->getNumCorners" << polygon->getNumCorners() << std::endl;
//    for(index_t i = nFacesInBoundary-5;i<nFacesInBoundary+1;++i) {
//        std::cerr << "el(" << i << ") = " << polygon->el().at(i) << std::endl;
//        for(index_t j = 0;j<1;++j) {
//            std::cerr << "cl(" << i*4+j << ") = " << polygon->cl().at(i*4+j) << std::endl;
//        }
//    }

    std::cerr << "polygon->el().at(polygon->getNumElements())"
                 ""
                 "" << polygon->el().at(polygon->getNumElements()) << std::endl;
    std::cerr << "polygon->getNumCorners()" << polygon->getNumCorners() << std::endl;
    std::cerr << "nconnectivities = " << ptrOnCl.size() << std::endl;
    std::cerr << "elemListCounter = " << elemListCounter << std::endl;

    for(index_t i=nFacesInBoundary-10;i<=nFacesInBoundary;++i) {
        std::cerr << "ptrOnEl[" << i << "] = " << ptrOnEl[i] << std::endl;
    }
    for(int i=elemListCounter-10;i<=elemListCounter;++i) {
        std::cerr << "ptrOnCl[" << i << "] = " << ptrOnCl[i] << std::endl;
    }

    cfxExportBoundaryFree(m_boundariesSelected[boundaryNr].ID);

    return polygon;
}

DataBase::ptr ReadCFX::loadField(int volumeNr, int variableID) {

    if(cfxExportZoneSet(m_volumesSelected[volumeNr].zoneFlag,NULL) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
//    std::cerr << "m_volumesSelected[volumeNr].ID = " << m_volumesSelected[volumeNr].ID << "; m_volumesSelected[volumeNr].zoneFlag = " << m_volumesSelected[volumeNr].zoneFlag << std::endl;
    index_t nnodesInVolume;
    int *nodeListOfVolume;
    nnodesInVolume = cfxExportVolumeSize(m_volumesSelected[volumeNr].ID,cfxVOL_NODES);
    nodeListOfVolume = cfxExportVolumeList(m_volumesSelected[volumeNr].ID,cfxVOL_NODES); //query the nodes that define the volume

    //read field parameters
    index_t varnum = variableID;

    if(m_case.m_ParamDimension[varnum] == 1) {
        Vec<Scalar>::ptr s(new Vec<Scalar>(nnodesInVolume));
        boost::shared_ptr<float_t> value(new float);
        //boost::shared_ptr<scalar_t> ptrOnScalarData(new scalar_t);
        scalar_t *ptrOnScalarData;
        ptrOnScalarData = s->x().data();
        for(index_t j=0;j<nnodesInVolume;++j) {
            cfxExportVariableGet(varnum,correct,nodeListOfVolume[j],value.get());
            ptrOnScalarData[j] = *value.get();
//            if(j<200) {
//                std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
//            }
        }
        cfxExportVariableFree(varnum);
        return s;
    }
    else if(m_case.m_ParamDimension[varnum] == 3) {
        Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nnodesInVolume));
        boost::shared_ptr<float_t> value(new float[3]);
        //boost::shared_ptr<scalar_t> ptrOnScalarData(new scalar_t);
        scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
        ptrOnVectorXData = v->x().data();
        ptrOnVectorYData = v->y().data();
        ptrOnVectorZData = v->z().data();
        for(index_t j=0;j<nnodesInVolume;++j) {
            cfxExportVariableGet(varnum,correct,nodeListOfVolume[j],value.get());
            ptrOnVectorXData[j] = value.get()[0];
            ptrOnVectorYData[j] = value.get()[1];
            ptrOnVectorZData[j] = value.get()[2];
//            if(j<2000) {
//                std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
//                std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
//                std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
//            }
        }
        cfxExportVariableFree(varnum);
        return v;
    }

    return DataBase::ptr();
}

DataBase::ptr ReadCFX::loadBoundaryField(int boundaryNr, int variableID) {
    DataBase::ptr result;


    //hier muss mit cfxExportBoundaryGet für jede vom User angegebene Boundary die Faces  ausgelesen werden
    //und dann mit cfxExportFaceNodes die Knoten

    //oder mit cfxExportBoundaryList die Knoten einer Boundary condition
    // und dann mit cfxExportVariableGet die Daten dazu
    return result;
}

int ReadCFX::collectVolumes() {
    // read zone selection; m_coRestraintZones contains a bool array of which zones are selected
        //m_coRestraintZones(zone) = 1 Zone ist selektiert
        //m_coRestraintZones(zone) = 0 Zone ist nicht selektiert
        //group = -1 alle Zonen sind selektiert
    m_coRestraintZones.clear();
    m_coRestraintZones.add(m_zoneSelection->getValue());
    ssize_t val = m_coRestraintZones.getNumGroups(), group;
    m_coRestraintZones.get(val,group);

    int numberOfSelectedVolumes=0;
    m_volumesSelected.reserve(m_nvolumes);

    for(index_t i=1;i<=m_nzones;++i) {
        if(m_coRestraintZones(i)) {
            if(cfxExportZoneSet(i,NULL)<0) {
                std::cerr << "invalid zone number" << std::endl;
                return numberOfSelectedVolumes;
            }
            int nvolumes = cfxExportVolumeCount();
            for(int j=1;j<=nvolumes;++j) {
                //std::cerr << "volumeName no. " << j << " in zone. " << i << " = " << cfxExportVolumeName(j) << std::endl;
                m_volumesSelected[numberOfSelectedVolumes]=IdWithZoneFlag(j,i);
                numberOfSelectedVolumes++;
            }
        }
    }

    //zum Testen
//    for(index_t i=0;i<numberOfSelectedVolumes;++i) {
//        std::cerr << "m_volumesSelected[" << i << "].ID = " << m_volumesSelected[i].ID << " m_volumesSelected.zoneFlag" << m_volumesSelected[i].zoneFlag << std::endl;
//    }
    cfxExportZoneFree();
    return numberOfSelectedVolumes;
}

int ReadCFX::collectBoundaries() {
        //m_coRestraintBoundaries contains a bool array of which boundaries are selected
        //m_coRestraintBoundaries(zone) = 1 boundary ist selektiert
        //m_coRestraintBoundaries(zone) = 0 boundary ist nicht selektiert
        //group = -1 alle boundaries sind selektiert
    m_coRestraintBoundaries.clear();
    m_coRestraintBoundaries.add(m_boundarySelection->getValue());
    ssize_t val = m_coRestraintBoundaries.getNumGroups(), group;
    m_coRestraintBoundaries.get(val,group);

    int numberOfSelectedBoundaries=0;
    m_boundariesSelected.reserve(m_nboundaries);

    for(index_t i=1;i<=m_nzones;++i) {
        cfxExportZoneSet(i,NULL);
        int nboundaries_in_zone = cfxExportBoundaryCount();
        for(int j=1;j<=nboundaries_in_zone;++j) {
            bm_type::right_const_iterator right_iter = m_case.m_allBoundaries.right.find(cfxExportBoundaryName(j));
            if(m_coRestraintBoundaries(right_iter->second)) {
                m_boundariesSelected[numberOfSelectedBoundaries]=IdWithZoneFlag(j,i);
                numberOfSelectedBoundaries++;
            }
        }
    }

    //zum Testen
//    for(int i=0;i<numberOfSelectedBoundaries;++i) {
//        std::cerr << "m_boundariesSelected[" << i << "] = " << m_boundariesSelected[i].ID << "; zoneflag = " << m_boundariesSelected[i].zoneFlag << std::endl;
//    }
    cfxExportZoneFree();
    return numberOfSelectedBoundaries;
}

bool ReadCFX::loadFields(int volumeNr) {
   for (int i=0; i<NumPorts; ++i) {
      std::string field = m_fieldOut[i]->getValue();
      bm_type::right_const_iterator right_iter = m_case.m_allParam.right.find(field);
      DataBase::ptr obj = loadField(volumeNr, right_iter->second);
//      if(!obj) {
//          std::cerr << "Data port number " << i << " (" << m_case.m_allParam.left.at(right_iter->second) << ") is empty - not a valid variable" << std::endl;
//      }
      //setMeta(obj, processor, timestep);
      m_currentVolumedata[i]= obj;
   }
   return true;
}

bool ReadCFX::loadBoundaryFields(int boundaryNr) {
    for (int i=0; i<NumBoundaryPorts; ++i) {
        std::string boundField = m_boundaryOut[i]->getValue();
        bm_type::right_const_iterator right_iter = m_case.m_allParam.right.find(boundField);
        DataBase::ptr obj = loadBoundaryField(boundaryNr, right_iter->second);
        //setMeta(obj, processor, timestep);
        //obj ->setGrid(loadPolygon(boundaryNr));
        obj ->setMapping(DataBase::Vertex);
        addObject(m_boundaryDataOut[i],obj);
    }
    return true;
}

bool ReadCFX::addVolumeDataToPorts(int volumeNr) {
    for (int portnum=0; portnum<NumPorts; ++portnum) {
        auto &volumedata = m_currentVolumedata;
        if(volumedata.find(portnum) != volumedata.end()) {
            if(volumedata[portnum]) {
                volumedata[portnum] ->setGrid(m_currentGrid[volumeNr]);
                volumedata[portnum] ->setMapping(DataBase::Vertex);
                addObject(m_volumeDataOut[portnum], volumedata[portnum]);
            }
        }
    }
   return true;
}

bool ReadCFX::addGridToPort(int volumeNr) {
    addObject(m_gridOut,m_currentGrid[volumeNr]);
    return true;
}

bool ReadCFX::compute() {

    std::cerr << "Compute Start. \n";

    if(!m_case.m_valid) {
        sendInfo("not a valid .res file entered: initialisation not yet done");
    }
    else {
        if(ExportDone) {
            parameterChanged(m_resultfiledir);
        }

        int numSelVolumes = collectVolumes();
        sendInfo("Schleife über Volumes Start");
        for(int i=0;i<numSelVolumes;++i) {
            m_currentGrid[i] = loadGrid(i);
            loadFields(i);
            addGridToPort(i);
            addVolumeDataToPorts(i);
        }
        sendInfo("Schleife über Volumes End");

        int numSelBoundaries = collectBoundaries();
        for(int i=0;i<numSelBoundaries;++i) {
            //loadBoundaryFields(i);
            loadPolygon(i);
        }

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

        m_case.m_ParamDimension.clear();
        //m_case.m_allParam.clear();
        //m_case.m_allBoundaries.clear();
        m_currentVolumedata.clear();
        m_currentBoundaryVolumedata.clear();
        m_currentGrid.clear();
        cfxExportDone();
        ExportDone = true;
    }

    return true;
}
