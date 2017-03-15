#include <string>
#include <sstream>
#include <vector>

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

//#define CFX_DEBUG

namespace bf = boost::filesystem;

MODULE_MAIN(ReadCFX)

using namespace vistle;

ReadCFX::ReadCFX(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadCFX", shmname, name, moduleID) {

    // file browser parameter
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/mnt/raid/home/hpcjwint/data/cfx/rohr/hlrs_002.res", Parameter::Directory);
    m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/data/eckerle/HLRS_Visualisierung_01122016/Betriebspunkt_250_3000/Configuration3_001.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/data/MundP/3d_Visualisierung_CFX/Transient_003.res", Parameter::Directory);

    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/home/jwinterstein/data/cfx/rohr/hlrs_002.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/home/jwinterstein/data/cfx/rohr/hlrs_inst_002.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfiledir", "CFX case directory","/home/jwinterstein/data/cfx/MundP_3d_Visualisierung/3d_Visualisierung_CFX/Transient_003.res", Parameter::Directory);

    // timestep parameters
    m_firsttimestep = addIntParameter("firstTimestep", "start reading the first step at this timestep number", 0);
    setParameterMinimum<Integer>(m_firsttimestep, 0);
    m_lasttimestep = addIntParameter("lastTimestep", "stop reading timesteps at this timestep number", 0);
    setParameterMinimum<Integer>(m_lasttimestep, 0);
    m_timeskip = addIntParameter("timeskip", "skip this many timesteps after reading one", 0);
    setParameterMinimum<Integer>(m_timeskip, 0);

    //zone selection
    m_zoneSelection = addStringParameter("zones","select zone numbers e.g. 1,4,6-10","all");

    // mesh ports
    m_gridOut = createOutputPort("grid_out1");
    m_polyOut = createOutputPort("poly_out1");

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
    m_2dAreaSelection = addStringParameter("2D area","select boundary or region numbers e.g. 1,4,6-10","all");

    // 2d data ports and 2d data choice parameters
    for (int i=0; i<Num2dPorts; ++i) {
       {// 2d data ports
          std::stringstream s;
          s << "data_2d_out" << i;
          m_2dDataOut.push_back(createOutputPort(s.str()));
       }
       {// 2d data choice parameters
          std::stringstream s;
          s << "Data2d" << i;
          auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
          std::vector<std::string> choices;
          choices.push_back("(NONE)");
          setParameterChoices(p, choices);
          m_2dOut.push_back(p);
       }
    }
    //m_buildGhostcellsParam = addIntParameter("build_ghostcells", "whether to build ghost cells", 1, Parameter::Boolean);ll
    m_portDatas.resize(NumPorts);
    m_2dPortDatas.resize(Num2dPorts);
    m_currentVolumedata.resize(NumPorts);
    m_current2dData.resize(Num2dPorts);
}

ReadCFX::~ReadCFX() {
    cfxExportDone();
    m_ExportDone = true;
}

IdWithZoneFlag::IdWithZoneFlag()
    : ID(0),
      zoneFlag(0) {

}

IdWithZoneFlag::IdWithZoneFlag(index_t r
               , index_t z)
    : ID(r),
      zoneFlag(z) {

}

Variable::Variable(std::string Name, int Dimension, int onlyMeaningful, int ID, int zone)
    : m_VarName(Name),
      m_VarDimension(Dimension),
      m_onlyMeaningfulOnBoundary(onlyMeaningful) {
    m_vectorIdwithZone.push_back(IdWithZoneFlag(ID,zone));
}

Boundary::Boundary(std::string Name, int ID, int zone)
    : m_BoundName(Name),
      m_IdwithZone(IdWithZoneFlag(ID,zone)) {

}

Region::Region(std::string Name, int ID, int zone)
    : m_RegionName(Name),
      m_IdwithZone(IdWithZoneFlag(ID,zone)) {

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
    int dimension, corrected_boundary_node, length;
    index_t nvars, nzones, nbounds, nregions;
    m_numberOfBoundaries = 0;
    m_numberOfRegions = 0;
    m_numberOfVariables = 0;

    nzones = cfxExportZoneCount();
    m_allParam.clear();
    m_allBoundaries.clear();

    for(index_t i=1;i<=nzones;++i) {
        cfxExportZoneSet(i,NULL);

        //read all Variable into m_allParam vector
        nvars = cfxExportVariableCount(ReadCFX::usr_level);
        //std::cerr << "nvars in zone(" << i << ") = " << nvars << std::endl;

        for(index_t varnum=1;varnum<=nvars;varnum++) {   //starts from 1 because cfxExportVariableName(varnum,ReadCFX::alias) only returnes values from 1 and higher
            const char *VariableName = cfxExportVariableName(varnum,ReadCFX::alias);
            auto it = find_if(m_allParam.begin(), m_allParam.end(), [&VariableName](const Variable& obj) {
                return obj.m_VarName == VariableName;
            });
            if (it == m_allParam.end()) {
                if(!cfxExportVariableSize(varnum,&dimension,&length,&corrected_boundary_node)) {
                    std::cerr << "variable if out of range in (parseResultfile -> cfxExportVariableSize)" << std::endl;
                }
                m_allParam.push_back(Variable(VariableName,dimension,length,varnum,i));
                m_numberOfVariables++;
            }
            else {
                auto index = std::distance(m_allParam.begin(), it);
                m_allParam[index].m_vectorIdwithZone.push_back(IdWithZoneFlag(varnum,i));
            }

            cfxExportVariableFree(varnum);
        }

        //read all Boundaries into m_allBoundaries vector
        nbounds = cfxExportBoundaryCount();
        for(index_t boundnum=1;boundnum<=nbounds;boundnum++) {
            m_allBoundaries.push_back(Boundary(cfxExportBoundaryName(boundnum),boundnum,i));
            m_numberOfBoundaries++;
        }

        //read all Boundaries into m_allBoundaries vector
        nregions = cfxExportRegionCount();
        for(index_t regnum=1;regnum<=nregions;regnum++) {
            m_allRegions.push_back(Region(cfxExportRegionName(regnum),regnum,i));
            m_numberOfRegions++;
        }
        cfxExportZoneFree();
    }

    //verification:
//    for(index_t i=0;i<m_allParam.size();++i) {
//        std::cerr << "m_allParam[" << i << "].m_VarName = " << m_allParam[i].m_VarName << std::endl;
//        std::cerr << "m_allParam[" << i << "].Dimension = " << m_allParam[i].m_VarDimension << std::endl;
//        std::cerr << "m_allParam[" << i << "].onlyMeaningful = " << m_allParam[i].m_onlyMeaningfulOnBoundary << std::endl;
//        for(index_t j=0;j<m_allParam[i].m_vectorIdwithZone.size();++j) {
//            std::cerr << "m_allParam[" << i << "].IdwZone.ID = " << m_allParam[i].m_vectorIdwithZone[j].ID << std::endl;
//            std::cerr << "m_allParam[" << i << "].IdwZone.zoneFlag = " << m_allParam[i].m_vectorIdwithZone[j].zoneFlag << std::endl;
//        }
//    }
//    for(index_t i=0;i<m_allBoundaries.size();++i) {
//        std::cerr << "m_allBoundaries[" << i << "].m_BoundName = " << m_allBoundaries[i].m_BoundName << std::endl;
//        std::cerr << "m_allBoundaries[" << i << "].IdwZone.ID = " << m_allBoundaries[i].m_IdwithZone.ID << std::endl;
//        std::cerr << "m_allBoundaries[" << i << "].IdwZone.zoneFlag = " << m_allBoundaries[i].m_IdwithZone.zoneFlag << std::endl;
//    }
//    for(index_t i=0;i<m_allRegions.size();++i) {
//        std::cerr << "m_allRegions[" << i << "].m_RegionName = " << m_allRegions[i].m_RegionName << std::endl;
//        std::cerr << "m_allRegions[" << i << "].IdwZone.ID = " << m_allRegions[i].m_IdwithZone.ID << std::endl;
//        std::cerr << "m_allRegions[" << i << "].IdwZone.zoneFlag = " << m_allRegions[i].m_IdwithZone.zoneFlag << std::endl;
//    }

    return;
}

void CaseInfo::getFieldList() {
    m_boundary_param.clear();
    m_boundary_param.push_back("(NONE)");
    m_field_param.clear();
    m_field_param.push_back("(NONE)");

    for(index_t varnum=0;varnum<m_numberOfVariables;varnum++) {
        if(m_allParam[varnum].m_onlyMeaningfulOnBoundary != 1) {
            m_field_param.push_back(m_allParam[varnum].m_VarName);
        }
        m_boundary_param.push_back(m_allParam[varnum].m_VarName);
    }
    return;
}

std::vector<Variable> CaseInfo::getCopyOfAllParam() {
    return m_allParam;
}

std::vector<Boundary> CaseInfo::getCopyOfAllBoundaries() {
    return m_allBoundaries;
}

std::vector<Region> CaseInfo::getCopyOfAllRegions() {
    return m_allRegions;
}

index_t CaseInfo::getNumberOfBoundaries() {
    return m_numberOfBoundaries;
}

index_t CaseInfo::getNumberOfRegions() {
    return m_numberOfRegions;
}

int ReadCFX::rankForVolumeAndTimestep(int timestep, int volume, int numVolumes) const {
    int processor = ((timestep-m_firsttimestep->getValue())/(m_timeskip->getValue()+1))*numVolumes+volume;

    return processor % size();
}

int ReadCFX::rankFor2dAreaAndTimestep(int timestep, int area2d, int num2dAreas) const {
    int processor = ((timestep-m_firsttimestep->getValue())/(m_timeskip->getValue()+1))*num2dAreas+area2d;

    return processor % size();
}

bool ReadCFX::initializeResultfile() {
    std::string c = m_resultfiledir->getValue();
    const char *resultfiledir;
    resultfiledir = c.c_str();

    char *resultfileName = strdup(resultfiledir);
    m_nzones = cfxExportInit(resultfileName, counts);
    m_nnodes = counts[cfxCNT_NODE];
    //m_nelems = counts[cfxCNT_ELEMENT];
    m_nregions = counts[cfxCNT_REGION];
    m_nvolumes = counts[cfxCNT_VOLUME];
    //m_nvars = counts[cfxCNT_VARIABLE];
    m_ExportDone = false;

    if (m_nzones < 0) {
        cfxExportDone();
        sendError("cfxExportInit could not open %s", resultfileName);
        m_ExportDone = true;
        return false;
    }

    free(resultfileName);
    return true;
}

bool ReadCFX::changeParameter(const Parameter *p) {

    if (p == m_resultfiledir) {
        std::string c = m_resultfiledir->getValue();
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
                m_ExportDone = true;
            }

            if(initializeResultfile()) {
                if (cfxExportTimestepNumGet(1) < 0) {
                    sendInfo("no timesteps");
                }
                m_ntimesteps = cfxExportTimestepCount();
                if(m_ntimesteps > 0) {
                    setParameterMaximum<Integer>(m_lasttimestep, m_ntimesteps-1);
                    setParameter<Integer>(m_lasttimestep,m_ntimesteps-1);
                    setParameterMaximum<Integer>(m_firsttimestep, m_ntimesteps-1);
                    setParameterMaximum<Integer>(m_timeskip, m_ntimesteps-1);
                }
                else {
                    setParameterMaximum<Integer>(m_lasttimestep, 0);
                    setParameter<Integer>(m_lasttimestep,0);
                    setParameterMaximum<Integer>(m_firsttimestep, 0);
                    setParameter<Integer>(m_firsttimestep,0);
                    setParameterMaximum<Integer>(m_timeskip, 0);
                }
                //fill choice parameter
                m_case.parseResultfile();
                m_case.getFieldList();
                for (auto out: m_fieldOut) {
                    setParameterChoices(out, m_case.m_field_param);
                }
                for (auto out: m_2dOut) {
                    setParameterChoices(out, m_case.m_boundary_param);
                }
                if(rank() == 0) {
                    //print out zone names
                    sendInfo("Found %d zones", m_nzones);
                    for(index_t i=1;i<=m_nzones;i++) {
                        cfxExportZoneSet(i,NULL);
                        sendInfo("zone no. %d: %s",i,cfxExportZoneName(i));
                        cfxExportZoneFree();
                    }
                    //print out 2D area names (boundaries and regions)
                    sendInfo("Found %d boundaries and %d regions", m_case.getNumberOfBoundaries(),m_case.getNumberOfRegions());
                    std::vector<Boundary> allBoundaries = m_case.getCopyOfAllBoundaries();
                    for(index_t i=1;i<=m_case.getNumberOfBoundaries();++i) {
                        sendInfo("boundary no. %d: %s",i,(allBoundaries[i-1].m_BoundName).c_str());
                    }
                    std::vector<Region> allRegions = m_case.getCopyOfAllRegions();
                    for(index_t i=(m_case.getNumberOfBoundaries()+1);i<=(m_case.getNumberOfBoundaries()+m_case.getNumberOfRegions());++i) {
                        sendInfo("region no. %d: %s",i,(allRegions[i-m_case.getNumberOfBoundaries()-1].m_RegionName).c_str());
                    }
                }
                cfxExportZoneFree();
#ifdef CFX_DEBUG
                sendInfo("The initialisation was successfully done");
#endif
            }
        }
    }
    return Module::changeParameter(p);
}

UnstructuredGrid::ptr ReadCFX::loadGrid(int volumeNr) {

    if(cfxExportZoneSet(m_volumesSelected[volumeNr].zoneFlag,counts) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    //std::cerr << "m_volumesSelected[volumeNr].ID = " << m_volumesSelected[volumeNr].ID << "; m_volumesSelected[volumeNr].zoneFlag = " << m_volumesSelected[volumeNr].zoneFlag << std::endl;

    index_t nnodesInVolume, nelmsInVolume, nconnectivities, nnodesInZone;
    nnodesInVolume = cfxExportVolumeSize(m_volumesSelected[volumeNr].ID,cfxVOL_NODES);
    nelmsInVolume = cfxExportVolumeSize(m_volumesSelected[volumeNr].ID,cfxVOL_ELEMS);
    nnodesInZone = cfxExportNodeCount();
    //std::cerr << "nodesInVolume = " << nnodesInVolume << "; nodesInZone = " << cfxExportNodeCount() << std::endl;
    //std::cerr << "nelmsInVolume = " << nelmsInVolume << "; tets = " << counts[cfxCNT_TET] << ", " << "pyramid = " << counts[cfxCNT_PYR] << ", "<< "prism = " << counts[cfxCNT_WDG] << ", "<< "hex = " << counts[cfxCNT_HEX] << std::endl;
    nconnectivities = 4*counts[cfxCNT_TET]+5*counts[cfxCNT_PYR]+6*counts[cfxCNT_WDG]+8*counts[cfxCNT_HEX];

    UnstructuredGrid::ptr grid(new UnstructuredGrid(nelmsInVolume, nconnectivities, nnodesInVolume)); //initialized with number of elements, number of connectivities, number of coordinates

    //load coords into unstructured grid
    boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);

    auto ptrOnXcoords = grid->x().data();
    auto ptrOnYcoords = grid->y().data();
    auto ptrOnZcoords = grid->z().data();

    int *nodeListOfVolume = cfxExportVolumeList(m_volumesSelected[volumeNr].ID,cfxVOL_NODES); //query the nodes that define the volume
    std::vector<std::int32_t> nodeListOfVolumeVec;
    nodeListOfVolumeVec.resize(nnodesInZone+1);

    for(index_t i=0;i<nnodesInVolume;++i) {
        cfxExportNodeGet(nodeListOfVolume[i],x_coord.get(),y_coord.get(),z_coord.get());   //get access to coordinates: [IN] nodeid [OUT] x,y,z
        ptrOnXcoords[i] = *x_coord.get();
        ptrOnYcoords[i] = *y_coord.get();
        ptrOnZcoords[i] = *z_coord.get();
        nodeListOfVolumeVec[nodeListOfVolume[i]] = i;
    }

    //Test, ob Einlesen funktioniert hat
    //        std::cerr << "m_nnodes = " << m_nnodes << std::endl;
    //        std::cerr << "grid->getNumCoords()" << grid->getNumCoords() << std::endl;
//    for(int i=0;i<20;++i) {
//            std::cerr << "x,y,z (" << i << ") = " << grid->x().at(i) << ", " << grid->y().at(i) << ", " << grid->z().at(i) << std::endl;
//    }
    //        std::cerr << "x,y,z (10)" << grid->x().at(10) << ", " << grid->y().at(10) << ", " << grid->z().at(10) << std::endl;
    //        std::cerr << "x,y,z (m_nnodes-1)" << grid->x().at(m_nnodes-1) << ", " << grid->y().at(m_nnodes-1) << ", " << grid->z().at(m_nnodes-1) << std::endl;


    //load element types, element list and connectivity list into unstructured grid
    int elemListCounter=0;
    boost::shared_ptr<std::int32_t> nodesOfElm(new int[8]), elemtype(new int);
    auto ptrOnTl = grid->tl().data();
    auto ptrOnEl = grid->el().data();
    auto ptrOnCl = grid->cl().data();

    int *elmListOfVolume = cfxExportVolumeList(m_volumesSelected[volumeNr].ID,cfxVOL_ELEMS); //query the elements that define the volume

    for(index_t i=0;i<nelmsInVolume;++i) {
        cfxExportElementGet(elmListOfVolume[i],elemtype.get(),nodesOfElm.get());
        switch(*elemtype.get()) {
            case 4: {
                ptrOnTl[i] = (UnstructuredGrid::TETRAHEDRON);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOfVolumeVec[nodesOfElm.get()[nodesOfElm_counter]];
                }
                elemListCounter += 4;
                break;
            }
            case 5: {
                ptrOnTl[i] = (UnstructuredGrid::PYRAMID);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<5;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOfVolumeVec[nodesOfElm.get()[nodesOfElm_counter]];
                }
                elemListCounter += 5;
                break;
            }
            case 6: {
                ptrOnTl[i] = (UnstructuredGrid::PRISM);
                ptrOnEl[i] = elemListCounter;

                // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
                ptrOnCl[elemListCounter+0] = nodeListOfVolumeVec[nodesOfElm.get()[3]];
                ptrOnCl[elemListCounter+1] = nodeListOfVolumeVec[nodesOfElm.get()[5]];
                ptrOnCl[elemListCounter+2] = nodeListOfVolumeVec[nodesOfElm.get()[4]];
                ptrOnCl[elemListCounter+3] = nodeListOfVolumeVec[nodesOfElm.get()[0]];
                ptrOnCl[elemListCounter+4] = nodeListOfVolumeVec[nodesOfElm.get()[2]];
                ptrOnCl[elemListCounter+5] = nodeListOfVolumeVec[nodesOfElm.get()[1]];
                elemListCounter += 6;
                break;
            }
            case 8: {
                ptrOnTl[i] = (UnstructuredGrid::HEXAHEDRON);
                ptrOnEl[i] = elemListCounter;

                // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
                ptrOnCl[elemListCounter+0] = nodeListOfVolumeVec[nodesOfElm.get()[4]];
                ptrOnCl[elemListCounter+1] = nodeListOfVolumeVec[nodesOfElm.get()[6]];
                ptrOnCl[elemListCounter+2] = nodeListOfVolumeVec[nodesOfElm.get()[7]];
                ptrOnCl[elemListCounter+3] = nodeListOfVolumeVec[nodesOfElm.get()[5]];
                ptrOnCl[elemListCounter+4] = nodeListOfVolumeVec[nodesOfElm.get()[0]];
                ptrOnCl[elemListCounter+5] = nodeListOfVolumeVec[nodesOfElm.get()[2]];
                ptrOnCl[elemListCounter+6] = nodeListOfVolumeVec[nodesOfElm.get()[3]];
                ptrOnCl[elemListCounter+7] = nodeListOfVolumeVec[nodesOfElm.get()[1]];

                elemListCounter += 8;
                break;
            }
            default: {
                std::cerr << "Elementtype(" << *elemtype.get() << "not yet implemented." << std::endl;
            }
        }
    }

    //element after last element
    ptrOnEl[nelmsInVolume] = elemListCounter;
    ptrOnCl[elemListCounter] = 0;

    grid->cl().resize(elemListCounter); //correct initialization; initialized with connectivities in zone; connectivities in volume are <= connectivities in zone

//    cfxElement *elems = cfxExportElementList();

//    int nelems = cfxExportElementCount();
//    for (int i = 0; i < nelems; i++, elems++) {
//        if(elems->type == 6 && i < 1680090) {
//            for (int j = 0; j < elems->type; j++) {
//                std::cerr << "elems->nodeid[" << j << "]-1 = " << elems->nodeid[j]-1 << std::endl;
//            }
//            std::cerr << "i = " << i << std::endl;
//            std::cerr << std::endl;
//        }
//    }


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
//    std::cerr << "elemListCounter = " << elemListCounter << std::endl;
//    std::cerr << "grid->getNumCorners()" << grid->getNumCorners() << std::endl;
//    std::cerr << "grid->getNumVertices()" << grid->getNumVertices() << std::endl;
//    std::cerr << "nelmsInVolume = " << nelmsInVolume << std::endl;
//    std::cerr << "nconnectivities = " << nconnectivities << std::endl;
//    std::cerr << "nnodesInVolume = " << nnodesInVolume << std::endl;

//    for(int i=nelmsInVolume-10;i<=nelmsInVolume;++i) {
//        std::cerr << "ptrOnEl[" << i << "] = " << ptrOnEl[i] << std::endl;
//    }
//    for(int i=elemListCounter-10;i<=elemListCounter;++i) {
//        std::cerr << "ptrOnCl[" << i << "] = " << ptrOnCl[i] << std::endl;
//    }
    cfxExportNodeFree();
    cfxExportElementFree();
    cfxExportVolumeFree(m_volumesSelected[volumeNr].ID);

    return grid;
}

Polygons::ptr ReadCFX::loadPolygon(int Area2d) {
    if(cfxExportZoneSet(m_2dAreasSelected[Area2d].m_IdWithZone.zoneFlag,counts) < 0) { //counts is a vector for statistics of the zone
        std::cerr << "invalid zone number" << std::endl;
    }

    int *nodeListOf2dArea;
    index_t nNodesInZone, nNodesIn2dArea, nConnectIn2dArea, nFacesIn2dArea;

    if(!strcmp((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(),"boundary")) {
        nNodesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES);
        nodeListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
        nFacesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_FACES);
    }
    else if(!strcmp((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(),"region")) {
        nNodesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES);
        nodeListOf2dArea = cfxExportRegionList(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
        nFacesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_FACES);
    }
    else {
        std::cerr << "error in loadPolygon: no valid 2d area" << std::endl;
    }

    nNodesInZone = cfxExportNodeCount();
    nConnectIn2dArea = 4*nFacesIn2dArea; //maximum of conncectivities. If there are 3 vertices faces, it is corrected with resize at the end of the function

    Polygons::ptr polygon(new Polygons(nFacesIn2dArea,nConnectIn2dArea,nNodesIn2dArea)); //initialize Polygon with numFaces, numCorners, numVertices

    //load coords into polygon
    boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);

    auto ptrOnXcoords = polygon->x().data();
    auto ptrOnYcoords = polygon->y().data();
    auto ptrOnZcoords = polygon->z().data();

    std::vector<std::int32_t> nodeListOf2dAreaVec;
    nodeListOf2dAreaVec.resize(nNodesInZone+1);

    for(index_t i=0;i<nNodesIn2dArea;++i) {
        cfxExportNodeGet(nodeListOf2dArea[i],x_coord.get(),y_coord.get(),z_coord.get());  //get access to coordinates: [IN] nodeid [OUT] x,y,z
        ptrOnXcoords[i] = *x_coord.get();
        ptrOnYcoords[i] = *y_coord.get();
        ptrOnZcoords[i] = *z_coord.get();
        nodeListOf2dAreaVec[nodeListOf2dArea[i]] = i;
    }

    //Test, ob Einlesen funktioniert hat
//        std::cerr << "nNodesIn2dArea = " << nNodesIn2dArea << std::endl;
//        std::cerr << "polygon->getNumCoords()" << polygon->getNumCoords() << std::endl;
//        for(int i=0;i<100;++i) {
//            std::cerr << "x,y,z (" << i << ") = " << polygon->x().at(i) << ", " << polygon->y().at(i) << ", " << polygon->z().at(i) << std::endl;
//        }

    cfxExportNodeFree();
    free2dArea((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(), Area2d);

    //attention: cfxExportBoundary/RegionList(cfxREG_FACES) has to be called AFTER cfxExportBoundary/RegionList(cfxREG_NODES) and free2dArea
    //if not, cfxExportBoundary/RegionList(cfxREG_NODES) gives unreasonable values for node id's
    int *faceListOf2dArea;
    if(!strcmp((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(),"boundary")) {
        faceListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_FACES); //query the faces that define the 2dArea
    }
    else if(!strcmp((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(),"region")) {
        faceListOf2dArea = cfxExportRegionList(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_FACES); //query the faces that define the 2dArea
    }
    else {
        std::cerr << "error in loadPolygon: no valid 2D area" << std::endl;
    }

    //load face types, element list and connectivity list into polygon
    int elemListCounter=0;
    boost::shared_ptr<std::int32_t> nodesOfFace(new int[4]);
    auto ptrOnEl = polygon->el().data();
    auto ptrOnCl = polygon->cl().data();

    for(index_t i=0;i<nFacesIn2dArea;++i) {
        int NumOfVerticesDefiningFace = cfxExportFaceNodes(faceListOf2dArea[i],nodesOfFace.get());
        assert(NumOfVerticesDefiningFace<=4); //see CFX Reference Guide p.57
        switch(NumOfVerticesDefiningFace) {
        case 3: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<3;++nodesOfElm_counter) {
                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOf2dAreaVec[nodesOfFace.get()[nodesOfElm_counter]];
            }
            elemListCounter += 3;
            break;
        }
        case 4: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOf2dAreaVec[nodesOfFace.get()[nodesOfElm_counter]];
            }
            elemListCounter += 4;
            break;
        }
        default: {
            std::cerr << "number of vertices defining the face(" << NumOfVerticesDefiningFace << ") not implemented." << std::endl;
        }
        }

//        if(i<20) {
//            std::cerr << "faceListOf2dArea[" << i << "] = " << faceListOf2dArea[i] << std::endl;
//            for(int j=0;j<4;++j) {
//                std::cerr << "nodesOfFace[" << j << "] = " << nodesOfFace.get()[j] << std::endl;
//            }
//            std::cerr << "local face number[" << faceListOf2dArea[i] << "] = " << cfxFACENUM(faceListOf2dArea[i]) << std::endl;
//            std::cerr << "local element number[" << faceListOf2dArea[i] << "] = " << cfxELEMNUM(faceListOf2dArea[i]) << std::endl;
//            std::cerr << "size of nodesOfFace = " << NumOfVerticesDefiningFace << std::endl;
//        }
    }

    //element after last element in element list and connectivity list
    ptrOnEl[nFacesIn2dArea] = elemListCounter;
    ptrOnCl[elemListCounter] = 0;
    polygon->cl().resize(elemListCounter);

    //Test, ob Einlesen funktioniert hat
//    std::cerr << "nodes = " << nNodesIn2dArea << "; faces = " << nFacesIn2dArea << "; connect = " << nConnectIn2dArea << std::endl;
//    std::cerr << "nodes_total = " << m_nnodes << "; node Count() = " << cfxExportNodeCount() << std::endl;
//    std::cerr << "nodes = " << nNodesIn2dArea << "; faces = " << nFacesIn2dArea << "; connect = " << polygon->cl().size() << std::endl;
//    std::cerr <<"polygon->getNumVertices = " << polygon->getNumVertices() << std::endl;
//    std::cerr <<"polygon->getNumElements = " << polygon->getNumElements() << std::endl;
//    std::cerr <<"polygon->getNumCorners = " << polygon->getNumCorners() << std::endl;

//    std::cerr << "polygon->el().at(polygon->getNumElements()) = " << polygon->el().at(polygon->getNumElements()) << std::endl;
//    std::cerr << "polygon->cl().at(polygon->getNumCorners()-1) = " << polygon->cl().at(polygon->getNumCorners()-1) << std::endl;
//    std::cerr << "elemListCounter = " << elemListCounter << std::endl;

//    for(index_t i=nFacesIn2dArea-10;i<=nFacesIn2dArea;++i) {
//        std::cerr << "ptrOnEl[" << i << "] = " << ptrOnEl[i] << std::endl;
//    }
//    for(int i=elemListCounter-10;i<=elemListCounter;++i) {
//        std::cerr << "ptrOnCl[" << i << "] = " << ptrOnCl[i] << std::endl;
//    }

    free2dArea((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(), Area2d);

    return polygon;
}

DataBase::ptr ReadCFX::loadField(int volumeNr, Variable var) {
    for(index_t i=0;i<var.m_vectorIdwithZone.size();++i) {
        if(var.m_vectorIdwithZone[i].zoneFlag == m_volumesSelected[volumeNr].zoneFlag) {
            if(cfxExportZoneSet(m_volumesSelected[volumeNr].zoneFlag,NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
            }
            //    std::cerr << "m_volumesSelected[volumeNr].ID = " << m_volumesSelected[volumeNr].ID << "; m_volumesSelected[volumeNr].zoneFlag = " << m_volumesSelected[volumeNr].zoneFlag << std::endl;
            index_t nnodesInVolume = cfxExportVolumeSize(m_volumesSelected[volumeNr].ID,cfxVOL_NODES);
            int *nodeListOfVolume = cfxExportVolumeList(m_volumesSelected[volumeNr].ID,cfxVOL_NODES); //query the nodes that define the volume

            //read field parameters
            index_t varnum = var.m_vectorIdwithZone[i].ID;

            if(var.m_VarDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nnodesInVolume));
                boost::shared_ptr<float_t> value(new float);
                scalar_t *ptrOnScalarData = s->x().data();

                for(index_t j=0;j<nnodesInVolume;++j) {
                    cfxExportVariableGet(varnum,correct,nodeListOfVolume[j],value.get());
                    ptrOnScalarData[j] = *value.get();
//                    if(j<10) {
//                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
//                    }
                }
                cfxExportVariableFree(varnum);
                cfxExportVolumeFree(m_volumesSelected[volumeNr].ID);
                return s;
            }
            else if(var.m_VarDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nnodesInVolume));
                boost::shared_ptr<float_t> value(new float[3]);
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();

                for(index_t j=0;j<nnodesInVolume;++j) {
                    cfxExportVariableGet(varnum,correct,nodeListOfVolume[j],value.get());
                    ptrOnVectorXData[j] = value.get()[0];
                    ptrOnVectorYData[j] = value.get()[1];
                    ptrOnVectorZData[j] = value.get()[2];
//                    if(j<20) {
//                        std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
//                        std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
//                        std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
//                    }
                }
                cfxExportVariableFree(varnum);
                cfxExportVolumeFree(m_volumesSelected[volumeNr].ID);
                return v;
            }
            cfxExportVolumeFree(m_volumesSelected[volumeNr].ID);
        }
    }
    return DataBase::ptr();
}

bool ReadCFX::free2dArea(const char *Area2dType, int Area2d) {
    if(!strcmp(Area2dType,"boundary")) {
        cfxExportBoundaryFree(m_2dAreasSelected[Area2d].m_IdWithZone.ID);
    }
    else if(!strcmp(Area2dType,"region")) {
        cfxExportRegionFree(m_2dAreasSelected[Area2d].m_IdWithZone.ID);
    }
    else {
        std::cerr << "Fehler free2dArea" << std::endl;
    }
    return true;
}

DataBase::ptr ReadCFX::load2dField(int Area2d, Variable var) {
    for(index_t i=0;i<var.m_vectorIdwithZone.size();++i) {
        if(m_2dAreasSelected[Area2d].m_IdWithZone.zoneFlag == var.m_vectorIdwithZone[i].zoneFlag) {
            if(cfxExportZoneSet(m_2dAreasSelected[Area2d].m_IdWithZone.zoneFlag,NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
            }
            index_t nNodesIn2dArea;
            int *nodeListOf2dArea;
            if(!strcmp((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(),"boundary")) {
                nNodesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES);
                nodeListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
            }
            else if(!strcmp((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(),"region")) {
                nNodesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES);
                nodeListOf2dArea = cfxExportRegionList(m_2dAreasSelected[Area2d].m_IdWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
            }
            else {
                std::cerr << "Fehler in load2dAreaField 1" << std::endl;
            }

            //read field parameters
            index_t varnum = var.m_vectorIdwithZone[i].ID;
            if(var.m_VarDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nNodesIn2dArea));
                boost::shared_ptr<float_t> value(new float);
                scalar_t *ptrOnScalarData = s->x().data();
                for(index_t j=0;j<nNodesIn2dArea;++j) {
                    cfxExportVariableGet(varnum,correct,nodeListOf2dArea[j],value.get());
                    ptrOnScalarData[j] = *value.get();
//                    if(j<100) {
//                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
//                    }
                }
                cfxExportVariableFree(varnum);
                free2dArea((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(), Area2d);
                return s;
            }
            else if(var.m_VarDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nNodesIn2dArea));
                boost::shared_ptr<float_t> value(new float[3]);
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();
                for(index_t j=0;j<nNodesIn2dArea;++j) {
                    cfxExportVariableGet(varnum,correct,nodeListOf2dArea[j],value.get());
                    ptrOnVectorXData[j] = value.get()[0];
                    ptrOnVectorYData[j] = value.get()[1];
                    ptrOnVectorZData[j] = value.get()[2];
//                    if(j<100) {
//                        std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
//                        std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
//                        std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
//                    }
                }
                cfxExportVariableFree(varnum);
                free2dArea((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(), Area2d);
                return v;
            }
            free2dArea((m_2dAreasSelected[Area2d].m_2dAreaType).c_str(), Area2d);
        }
    }
    return DataBase::ptr();
}

index_t ReadCFX::collectVolumes() {
    // read zone selection; m_coRestraintZones contains a bool array of which zones are selected
        //m_coRestraintZones(zone) = 1 Zone ist selektiert
        //m_coRestraintZones(zone) = 0 Zone ist nicht selektiert
        //group = -1 alle Zonen sind selektiert
    m_coRestraintZones.clear();
    m_coRestraintZones.add(m_zoneSelection->getValue());
    ssize_t val = m_coRestraintZones.getNumGroups(), group;
    m_coRestraintZones.get(val,group);

    index_t numberOfSelectedVolumes=0;
    m_volumesSelected.resize(m_nvolumes);

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
    m_volumesSelected.resize(numberOfSelectedVolumes);

    //zum Testen
//    for(index_t i=0;i<numberOfSelectedVolumes;++i) {
//        std::cerr << "m_volumesSelected[" << i << "].ID = " << m_volumesSelected[i].ID << " m_volumesSelected.zoneFlag" << m_volumesSelected[i].zoneFlag << std::endl;
//    }
    cfxExportZoneFree();
    return numberOfSelectedVolumes;
}

index_t ReadCFX::collect2dAreas() {
    //m_coRestraint2dAreas contains a bool array of which 2dAreas are selected
    //m_coRestraint2dAreas(zone) = 1 2dAreas ist selektiert
    //m_coRestraint2dAreas(zone) = 0 2dAreas ist nicht selektiert
    //group = -1 alle 2dAreas sind selektiert
    m_coRestraint2dAreas.clear();
    m_coRestraint2dAreas.add(m_2dAreaSelection->getValue());
    ssize_t val = m_coRestraint2dAreas.getNumGroups(), group;
    m_coRestraint2dAreas.get(val,group);

    index_t numberOfSelected2dAreas=0;
    m_2dAreasSelected.resize(m_case.getNumberOfBoundaries()+m_case.getNumberOfRegions());
    std::vector<Boundary> allBoundaries = m_case.getCopyOfAllBoundaries();
    std::vector<Region> allRegions = m_case.getCopyOfAllRegions();

    for(index_t i=1;i<=m_case.getNumberOfBoundaries();++i) {
        if(m_coRestraint2dAreas(i)) {
            m_2dAreasSelected[numberOfSelected2dAreas].m_IdWithZone=IdWithZoneFlag(allBoundaries[i-1].m_IdwithZone.ID,allBoundaries[i-1].m_IdwithZone.zoneFlag);
            m_2dAreasSelected[numberOfSelected2dAreas].m_2dAreaType = "boundary";
            numberOfSelected2dAreas++;
        }
    }
    for(index_t i=m_case.getNumberOfBoundaries()+1;i<=(m_case.getNumberOfBoundaries()+m_case.getNumberOfRegions());++i) {
        if(m_coRestraint2dAreas(i)) {
            m_2dAreasSelected[numberOfSelected2dAreas].m_IdWithZone=IdWithZoneFlag(allRegions[i-m_case.getNumberOfBoundaries()-1].m_IdwithZone.ID,allRegions[i-m_case.getNumberOfBoundaries()-1].m_IdwithZone.zoneFlag);
            m_2dAreasSelected[numberOfSelected2dAreas].m_2dAreaType = "region";
            numberOfSelected2dAreas++;
        }
    }

    m_2dAreasSelected.resize(numberOfSelected2dAreas);

    //zum Testen
//    if(rank()==0) {
//        for(index_t i=0;i<numberOfSelected2dAreas;++i) {
//            std::cerr << "m_2dAreasSelected[" << i << "] = " << m_2dAreasSelected[i].m_IdWithZone.ID << "; zoneflag = " << m_2dAreasSelected[i].m_IdWithZone.zoneFlag << std::endl;
//        }
//    }

    cfxExportZoneFree();
    return numberOfSelected2dAreas;
}

bool ReadCFX::loadFields(int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes) {
   for (int i=0; i<NumPorts; ++i) {
      std::string field = m_fieldOut[i]->getValue();
      std::vector<Variable> allParam = m_case.getCopyOfAllParam();
      auto it = find_if(allParam.begin(), allParam.end(), [&field](const Variable& obj) {
          return obj.m_VarName == field;});
      if (it == allParam.end()) {
          if(!m_portDatas[i].m_vectorResfileVolumeData.empty()) {
              //values are only in resfile --> timestep = -1
              setDataObject(m_ResfileGridVec.back(),m_portDatas[i].m_vectorResfileVolumeData.back(),m_portDatas[i].m_vectorVolumeDataVolumeNr.back(),setMetaTimestep,-1,numSelVolumes,trnOrRes);
          }
          else {
              m_currentVolumedata[i] = DataBase::ptr();
          }
      }
      else {
          if(!m_portDatas[i].m_vectorResfileVolumeData.empty() && trnOrRes) {
              //resfile is last timestep
              setDataObject(m_ResfileGridVec.back(),m_portDatas[i].m_vectorResfileVolumeData.back(),m_portDatas[i].m_vectorVolumeDataVolumeNr.back(),0,0,numSelVolumes,0);
              return true;
          }
          auto index = std::distance(allParam.begin(), it);
          DataBase::ptr obj = loadField(volumeNr, allParam[index]);
          if(trnOrRes) {
              setDataObject(m_gridsInTimestep[volumeNr],obj,volumeNr,setMetaTimestep,timestep,numSelVolumes,trnOrRes);
              m_currentVolumedata[i]= obj;
          }
          else {
              m_portDatas[i].m_vectorResfileVolumeData.push_back(obj);
              m_portDatas[i].m_vectorVolumeDataVolumeNr.push_back(volumeNr);
              if(m_ntimesteps==0) {
                  setDataObject(m_ResfileGridVec.back(),m_portDatas[i].m_vectorResfileVolumeData.back(),volumeNr,setMetaTimestep,-1,numSelVolumes,trnOrRes);
              }
          }
      }
   }
   return true;
}

bool ReadCFX::load2dFields(int Area2d, int setMetaTimestep, int timestep, index_t numSel2dArea, bool trnOrRes) {
    for (int i=0; i<Num2dPorts; ++i) {
        std::string Area2dField = m_2dOut[i]->getValue();
        std::vector<Variable> allParam = m_case.getCopyOfAllParam();
        auto it = find_if(allParam.begin(), allParam.end(), [&Area2dField](const Variable& obj) {
            return obj.m_VarName == Area2dField;});
        if (it == allParam.end()) {
            if(!m_2dPortDatas[i].Resfile2dDataVec.empty()) {
                //values are only in resfile --> timestep = -1
                set2dObject(m_ResfilePolygonVec.back(),m_2dPortDatas[i].Resfile2dDataVec.back(),m_2dPortDatas[i].Resfile2dIdVec.back(),setMetaTimestep,-1,numSel2dArea,trnOrRes);
            }
            else {
               m_current2dData[i] = DataBase::ptr();
            }
       }
       else {
            if(!m_2dPortDatas[i].Resfile2dDataVec.empty() && trnOrRes) {
                //resfile is last timestep
                set2dObject(m_ResfilePolygonVec.back(),m_2dPortDatas[i].Resfile2dDataVec.back(),m_2dPortDatas[i].Resfile2dIdVec.back(),0,0,numSel2dArea,0);
                return true;
           }
           auto index = std::distance(allParam.begin(), it);
           DataBase::ptr obj = load2dField(Area2d, allParam[index]);
           if(trnOrRes) {
               set2dObject(m_polygonsInTimestep[Area2d],obj,Area2d,setMetaTimestep,timestep,numSel2dArea,trnOrRes);
               m_current2dData[i]= obj;
           }
           else {
               m_2dPortDatas[i].Resfile2dDataVec.push_back(obj);
               m_2dPortDatas[i].Resfile2dIdVec.push_back(Area2d);
               if(m_ntimesteps==0) {
                   set2dObject(m_ResfilePolygonVec.back(),m_2dPortDatas[i].Resfile2dDataVec.back(),Area2d,setMetaTimestep,-1,numSel2dArea,trnOrRes);
               }
           }
       }
    }
    return true;
}

bool ReadCFX::addVolumeDataToPorts() {
    for (int portnum=0; portnum<NumPorts; ++portnum) {
        if(!m_portDatas[portnum].m_vectorResfileVolumeData.empty()) {
            auto &volumedata = m_portDatas[portnum].m_vectorResfileVolumeData.back();
            if(volumedata) {
                //std::cerr << "addVolume(" << portnum << ")" << std::endl;
                addObject(m_volumeDataOut[portnum], volumedata);
            }
            m_portDatas[portnum].m_vectorResfileVolumeData.pop_back();
            m_portDatas[portnum].m_vectorVolumeDataVolumeNr.pop_back();
        }
        else {
            if(m_currentVolumedata[portnum]) {
                //std::cerr << "addVolume(" << portnum << ")" << std::endl;
                addObject(m_volumeDataOut[portnum], m_currentVolumedata[portnum]);
            }
        }
    }
   return true;
}

bool ReadCFX::add2dDataToPorts() {
    for (int portnum=0; portnum<Num2dPorts; ++portnum) {
        if(!m_2dPortDatas[portnum].Resfile2dDataVec.empty()) {
            auto &data2d = m_2dPortDatas[portnum].Resfile2dDataVec.back();
            if(data2d) {
                //std::cerr << "addVolume(" << portnum << ")" << std::endl;
                addObject(m_2dDataOut[portnum], data2d);
            }
            m_2dPortDatas[portnum].Resfile2dDataVec.pop_back();
            m_2dPortDatas[portnum].Resfile2dIdVec.pop_back();
        }
        else {
            if(m_current2dData[portnum]) {
                addObject(m_2dDataOut[portnum], m_current2dData[portnum]);
            }
        }
    }
    return true;
}

bool ReadCFX::addGridToPort(int volumeNr) {
    if(!m_ResfileGridVec.empty()) {
        addObject(m_gridOut,m_ResfileGridVec.back());
        m_ResfileGridVec.pop_back();
    }
    else {
        addObject(m_gridOut,m_gridsInTimestep[volumeNr]);
    }
    return true;
}

bool ReadCFX::addPolygonToPort(int Area2d) {
    if(!m_ResfilePolygonVec.empty()) {
        addObject(m_polyOut,m_ResfilePolygonVec.back());
        m_ResfilePolygonVec.pop_back();
    }
    else {
        addObject(m_polyOut,m_polygonsInTimestep[Area2d]);
    }
    return true;
}

void ReadCFX::setMeta(Object::ptr obj, int blockNr, int setMetaTimestep, int timestep, index_t totalBlockNr, bool trnOrRes) {
   if (obj) {
       if(m_ntimesteps == 0) {
           obj->setTimestep(-1);
           obj->setNumTimesteps(-1);
           obj->setRealTime(0);
       }
       else {
           if(timestep == -1) {
               obj->setNumTimesteps(-1);
               obj->setTimestep(-1);
           }
           else {
               if(trnOrRes) {
                   obj->setNumTimesteps(((m_lasttimestep->getValue()-m_firsttimestep->getValue())/(m_timeskip->getValue()+1))+2);
                   obj->setTimestep(setMetaTimestep);
                   obj->setRealTime(cfxExportTimestepTimeGet(timestep+1)); //+1 because cfxExport API's start with Index 1
               }
               else {
                   obj->setNumTimesteps(((m_lasttimestep->getValue()-m_firsttimestep->getValue())/(m_timeskip->getValue()+1))+2);
                   obj->setTimestep(((m_lasttimestep->getValue()-m_firsttimestep->getValue())/(m_timeskip->getValue()+1))+1);
                   obj->setRealTime(cfxExportTimestepTimeGet(m_lasttimestep->getValue()+1)+cfxExportTimestepTimeGet(1));  //+1 because cfxExport API's start with Index 1
               }
           }
       }
      obj->setBlock(blockNr);
      obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);


      double rotAxis[2][3] = {{0.0, 0.0, 0.0},{0.0, 0.0, 0.0}};
      double angularVel; //in radians per second
      if(cfxExportZoneIsRotating(rotAxis,&angularVel)) { //1 if zone is rotating, 0 if zone is not rotating
          cfxExportZoneMotionAction(cfxExportZoneGet(),cfxMOTION_USE);

//          for(int i=0;i<3;++i) {
//              std::cerr << "rotAxis[0][" << i << "] = " << rotAxis[0][i] << std::endl;
//          }
//          for(int i=0;i<3;++i) {
//              std::cerr << "rotAxis[1][" << i << "] = " << rotAxis[1][i] << std::endl;
//          }

          double startTime = cfxExportTimestepTimeGet(1);
          double currentTime = cfxExportTimestepTimeGet(timestep+1);
          Scalar rot_angle = angularVel*(currentTime-startTime); //in radians

          //std::cerr << "rot_angle = " << rot_angle << std::endl;

          double x,y,z;
          x = rotAxis[1][0]-rotAxis[0][0];
          y = rotAxis[1][1]-rotAxis[0][1];
          z = rotAxis[1][2]-rotAxis[0][2];
          Vector3 rot_axis(x,y,z);
          rot_axis.normalize();
          //std::cerr << "rot_axis = " << rot_axis << std::endl;

          Quaternion qrot(AngleAxis(rot_angle, rot_axis));
          Matrix4 rotMat(Matrix4::Identity());
          Matrix3 rotMat3(qrot.toRotationMatrix());
          rotMat.block<3,3>(0,0) = rotMat3;

          Vector3 translate(-rotAxis[0][0],-rotAxis[0][1],-rotAxis[0][2]);
          Matrix4 translateMat(Matrix4::Identity());
          translateMat.col(3) << translate, 1;

          Matrix4 transform(Matrix4::Identity());
          transform *= translateMat;
          transform *= rotMat;
          translate *= -1;
          translateMat.col(3) << translate, 1;
          transform *= translateMat;

          Matrix4 t = obj->getTransform();
          t *= transform;
          obj->setTransform(t);
      }
   }
}

bool ReadCFX::setDataObject(UnstructuredGrid::ptr grid, DataBase::ptr data, int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes) {
    setMeta(grid,volumeNr,setMetaTimestep,timestep,numSelVolumes,trnOrRes);
    setMeta(data,volumeNr,setMetaTimestep,timestep,numSelVolumes,trnOrRes);
    data->setGrid(grid);
    data->setMapping(DataBase::Vertex);

    return true;
}

bool ReadCFX::set2dObject(Polygons::ptr polygon, DataBase::ptr data, int Area2d, int setMetaTimestep, int timestep, index_t numSel2dArea, bool trnOrRes) {
    setMeta(polygon,Area2d,setMetaTimestep,timestep,numSel2dArea,trnOrRes);
    setMeta(data,Area2d,setMetaTimestep,timestep,numSel2dArea,trnOrRes);
    data->setGrid(polygon);
    data->setMapping(DataBase::Vertex);

    return true;
}

bool ReadCFX::readTime(index_t numSelVolumes, index_t numSel2dArea, int setMetaTimestep, int timestep, bool trnOrRes) {
    for(index_t i=0;i<numSelVolumes;++i) {
        //std::cerr << "rankForVolumeAndTimestep(" << timestep << "," << firsttimestep << "," << step << "," << i << "," << numSelVolumes << ") = " << rankForVolumeAndTimestep(timestep,firsttimestep,step,i,numSelVolumes) << std::endl;
        if(rankForVolumeAndTimestep(timestep,i,numSelVolumes) == rank()) {                    
            //std::cerr << "process mit rank() = " << rank() << "; berechnet volume = " << i << "; in timestep = " << timestep << std::endl;
            if(m_ntimesteps==0) {
                m_ResfileGridVec.push_back(loadGrid(i));
                loadFields(i, setMetaTimestep, timestep, numSelVolumes, trnOrRes);
                addGridToPort(i);
                addVolumeDataToPorts();
            }
            else {
                if(trnOrRes) {
                    if(m_addToPortResfileVolumeData) {
                        for(int j=0;j<NumPorts;++j) {
                            if(!m_portDatas[j].m_vectorResfileVolumeData.empty()) {
                                index_t nResfileDataSets = m_portDatas[j].m_vectorResfileVolumeData.size();
                                for(index_t k=0;k<nResfileDataSets;++k) {
                                    loadFields(0, setMetaTimestep, timestep, numSelVolumes, trnOrRes);
                                    addGridToPort(i);
                                    addVolumeDataToPorts();
                                }
                            }
                        }
                        m_addToPortResfileVolumeData=false;
                    }
                    if(cfxExportZoneSet(m_volumesSelected[i].zoneFlag,NULL) < 0) { //set the right zone because cfxExportGridChanged depends on the zone
                        std::cerr << "invalid zone number" << std::endl;
                    }
                    if(!m_gridsInTimestep[i] || cfxExportGridChanged(m_previousTimestep,timestep+1)) {
                        m_gridsInTimestep[i] = loadGrid(i);
                    }
                    m_gridsInTimestep[i] = loadGrid(i);
                    loadFields(i, setMetaTimestep, timestep, numSelVolumes, trnOrRes);
                    addGridToPort(i);
                    addVolumeDataToPorts();
                }
                else {
                    m_ResfileGridVec.push_back(loadGrid(i));
                    loadFields(i, setMetaTimestep, timestep, numSelVolumes, trnOrRes);
                }
            }
        }
    }

    for(index_t i=0;i<numSel2dArea;++i) {
        if(rankFor2dAreaAndTimestep(timestep,i,numSel2dArea) == rank()) {
#ifdef CFX_DEBUG
            std::cerr << "process mit rank() = " << rank() << "; berechnet Area2d = " << i << "; in timestep = " << timestep << std::endl;
#endif
            if(m_ntimesteps==0) {
                m_ResfilePolygonVec.push_back(loadPolygon(i));
                load2dFields(i,setMetaTimestep, timestep, numSel2dArea, trnOrRes);
                addPolygonToPort(i);
                add2dDataToPorts();
            }
            else {
                if(trnOrRes) {
                    if(m_addToPortResfile2dData) {
                        for(int j=0;j<Num2dPorts;++j) {
                            if(!m_2dPortDatas[j].Resfile2dDataVec.empty()) {
                                int nResfileDataSets = m_2dPortDatas[j].Resfile2dDataVec.size();
                                for(int k=0;k<nResfileDataSets;++k) {
                                    load2dFields(0, setMetaTimestep, timestep, numSel2dArea, trnOrRes);
                                    addPolygonToPort(0);
                                    add2dDataToPorts();
                                }
                            }
                        }
                        m_addToPortResfile2dData=false;
                    }
                    if(!m_polygonsInTimestep[i] || cfxExportGridChanged(m_previousTimestep,timestep+1)) {
                        m_polygonsInTimestep[i] = loadPolygon(i);
                    }
                    m_polygonsInTimestep[i] = loadPolygon(i);
                    load2dFields(i, setMetaTimestep, timestep, numSel2dArea, trnOrRes);
                    addPolygonToPort(i);
                    add2dDataToPorts();
                }
                else {
                    m_ResfilePolygonVec.push_back(loadPolygon(i));
                    load2dFields(i, setMetaTimestep, timestep, numSel2dArea, trnOrRes);
                }
            }
        }
    }
    m_previousTimestep = timestep+1;
    return true;
}

bool ReadCFX::clearResfileData () {
    for(int i=0;i<NumPorts;++i) {
        m_portDatas[i].m_vectorResfileVolumeData.clear();
        m_portDatas[i].m_vectorVolumeDataVolumeNr.clear();
    }
    for(int i=0;i<Num2dPorts;++i) {
        m_2dPortDatas[i].Resfile2dDataVec.clear();
        m_2dPortDatas[i].Resfile2dIdVec.clear();
    }
    m_ResfileGridVec.clear();

    return true;
}

bool ReadCFX::compute() {
#ifdef CFX_DEBUG
    std::cerr << "Compute Start. \n";
#endif
    index_t numSelVolumes, numSel2dArea, setMetaTimestep=0;
    index_t step = m_timeskip->getValue()+1;
    index_t firsttimestep = m_firsttimestep->getValue(), lasttimestep =  m_lasttimestep->getValue();
    bool trnOrRes;

    if(!m_case.m_valid) {
        sendInfo("not a valid .res file entered: initialisation not yet done");
    }
    else {
        if(m_ExportDone) {
            initializeResultfile();
            m_case.parseResultfile();
        }
        //read variables out of .res file
        trnOrRes = 0;
#ifdef CFX_DEBUG
        std::cerr << "Test1" << std::endl;
#endif
        m_addToPortResfileVolumeData = true;
        m_addToPortResfile2dData = true;
        clearResfileData();
#ifdef CFX_DEBUG
        std::cerr << "Test2" << std::endl;
#endif
        numSelVolumes = collectVolumes();
        numSel2dArea = collect2dAreas();
        m_gridsInTimestep.resize(numSelVolumes);
        m_polygonsInTimestep.resize(numSel2dArea);
        readTime(numSelVolumes,numSel2dArea,0,0,trnOrRes);
#ifdef CFX_DEBUG
        std::cerr << "Test3" << std::endl;
        std::cerr << "numSel2dArea = " << numSel2dArea << std::endl;
#endif

        //read variables out of timesteps .trn file
        if(m_ntimesteps!=0) {
            trnOrRes = 1;
            for(index_t timestep = firsttimestep; timestep<=lasttimestep; timestep+=step) {
                index_t timestepNumber = cfxExportTimestepNumGet(timestep+1);
                //cfxExportTimestepTimeGet(timestepNumber);
                if(cfxExportTimestepSet(timestepNumber)<0) {
                    std::cerr << "cfxExportTimestepSet: invalid timestep number(" << timestepNumber << ")" << std::endl;
                }
                else {
                    if(rank() == 0 && timestep == firsttimestep) {
                        //print variable names in .trn file
                        sendInfo("Found %d variables in transient files", cfxExportVariableCount(usr_level));
                        for(int i=1;i<=cfxExportVariableCount(usr_level);i++) {
                            sendInfo("%d. %s",i,cfxExportVariableName(i,alias));
                        }
                    }
#ifdef CFX_DEBUG
                    std::cerr << "Test4" << std::endl;
#endif

                    m_case.parseResultfile();
                    numSelVolumes = collectVolumes();
                    numSel2dArea = collect2dAreas();
                    readTime(numSelVolumes,numSel2dArea,setMetaTimestep,timestep,trnOrRes);
#ifdef CFX_DEBUG
                    std::cerr << "Test5" << std::endl;
#endif
                }
                setMetaTimestep++;
            }
            cfxExportDone();
            m_ExportDone = true;
        }
    }
    clearResfileData();
#ifdef CFX_DEBUG
                    std::cerr << "Test6" << std::endl;
#endif
    return true;
}
