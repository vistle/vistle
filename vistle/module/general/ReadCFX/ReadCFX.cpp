#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/triangles.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/normals.h>
#include <core/unstr.h>
#include <core/lines.h>

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

#include <util/stopwatch.h>


#include "ReadCFX.h"

//#define CFX_DEBUG
#define PARALLEL_ZONES


namespace bf = boost::filesystem;

MODULE_MAIN(ReadCFX)

using namespace vistle;

ReadCFX::ReadCFX(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadCFX", shmname, name, moduleID) {

    // file browser parameter
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/mnt/raid/home/hpcjwint/data/cfx/rohr/hlrs_002.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/data/eckerle/HLRS_Visualisierung_01122016/Betriebspunkt_250_3000/Configuration3_001.res", Parameter::Directory);
    m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/data/MundP/3d_Visualisierung_CFX/Transient_003.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/mnt/raid/data/IET/AXIALZYKLON/120929_ML_AXIALZYKLON_P160_OPT_SSG_AB_V2_STATIONAER/Steady_grob_V44_P_test_160_5percent_001.res", Parameter::Directory);

    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/home/jwinterstein/data/cfx/rohr/hlrs_002.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/home/jwinterstein/data/cfx/rohr/hlrs_inst_002.res", Parameter::Directory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/home/jwinterstein/data/cfx/MundP_3d_Visualisierung/3d_Visualisierung_CFX/Transient_003.res", Parameter::Directory);

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
            s << "data" << i;
            auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back("(NONE)");
            setParameterChoices(p, choices);
            m_fieldOut.push_back(p);
        }
    }
    //m_readBoundary = addIntParameter("read_boundary", "load the boundary?", 0, Parameter::Boolean);
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
          s << "data2d" << i;
          auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
          std::vector<std::string> choices;
          choices.push_back("(NONE)");
          setParameterChoices(p, choices);
          m_2dOut.push_back(p);
       }
    }

//particle selection
    m_particleSelection = addStringParameter("particle type","select particle type e.g. 1,4,6-10","all");
    m_particleTime = createOutputPort("particle_time");

    // particle data ports and particle choice parameters
    for (int i=0; i<NumParticlePorts; ++i) {
       {// particle data ports
          std::stringstream s;
          s << "data_particle_out" << i;
          m_particleDataOut.push_back(createOutputPort(s.str()));
       }
       {// particle choice parameters
          std::stringstream s;
          s << "dataParticle" << i;
          auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
          std::vector<std::string> choices;
          choices.push_back("(NONE)");
          setParameterChoices(p, choices);
          m_particleOut.push_back(p);
       }
    }
    m_currentVolumedata.resize(NumPorts);
    m_current2dData.resize(Num2dPorts);
    m_currentParticleData.resize(NumParticlePorts+1);
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
    : varName(Name),
      varDimension(Dimension),
      onlyMeaningfulOnBoundary(onlyMeaningful) {
    vectorIdwithZone.push_back(IdWithZoneFlag(ID,zone));
}

Particle::Particle(std::string Type, std::string Name, int Dimension, int ID, int zone)
    : particleType(Type),
      varName(Name),
      varDimension(Dimension),
      idWithZone(IdWithZoneFlag(ID,zone)) {

}

Boundary::Boundary(std::string Name, int ID, int zone)
    : boundName(Name),
      idWithZone(IdWithZoneFlag(ID,zone)) {

}

Region::Region(std::string Name, int ID, int zone)
    : regionName(Name),
      idWithZone(IdWithZoneFlag(ID,zone)) {

}

CaseInfo::CaseInfo()
    : m_valid(false) {

}

bool CaseInfo::checkFile(const char *filename) {
    //checks if the file is a valid CFX result file. Every CFX result file starts with "*INFO"

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

bool CaseInfo::checkWhichVariablesAreInTransientFile(index_t ntimesteps) {
    //checks if a variable is in the transient files available or not. Not every variable out of the .res file must be in the .trn files as well.
    //mostly because of memory reasons, the .trn files have less variables

    m_variableInTransientFile.clear();
    if(ntimesteps > 0) {
        cfxExportTimestepSet(cfxExportTimestepNumGet(1));
        index_t nvars = cfxExportVariableCount(ReadCFX::usr_level);

        for(index_t varnum=1;varnum<=nvars;varnum++) {   //starts from 1 because cfxExportVariableName(varnum,ReadCFX::alias) only returnes values from 1 and higher
            m_variableInTransientFile.push_back(cfxExportVariableName(varnum,ReadCFX::alias));
            cfxExportVariableFree(varnum);
        }
    }


    //verification:
//    for(index_t i=0;i<m_allParam.size();++i) {
//        std::cerr << "m_allParam[" << i << "].varName = " << m_allParam[i].varName << std::endl;
//        std::cerr << "m_allParam[" << i << "].existsOnlyInResfile = " << m_allParam[i].existsOnlyInResfile << std::endl;
//        std::cerr << "m_allParam[" << i << "].Dimension = " << m_allParam[i].varDimension << std::endl;
//        std::cerr << "m_allParam[" << i << "].onlyMeaningful = " << m_allParam[i].onlyMeaningfulOnBoundary << std::endl;
//        for(index_t j=0;j<m_allParam[i].vectorIdwithZone.size();++j) {
//            std::cerr << "m_allParam[" << i << "].IdwZone.ID = " << m_allParam[i].vectorIdwithZone[j].ID << std::endl;
//            std::cerr << "m_allParam[" << i << "].IdwZone.zoneFlag = " << m_allParam[i].vectorIdwithZone[j].zoneFlag << std::endl;
//        }
//    }
    return true;
}

void CaseInfo::parseResultfile() {
    //check how many variables are available and store them together with their specific ID and zone flag
    //store all region and boundary names

    int dimension, corrected_boundary_node, length;
    index_t nvars, nzones, nbounds, nregions;
    m_numberOfBoundaries = 0;
    m_numberOfRegions = 0;
    m_numberOfVariables = 0;
    m_numberOfParticles = 0;

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
                return obj.varName == VariableName;
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
                m_allParam[index].vectorIdwithZone.push_back(IdWithZoneFlag(varnum,i));
            }
            cfxExportVariableFree(varnum);
        }

        //read all particles into m_allParticle vector
        index_t nparticleTypes = cfxExportGetNumberOfParticleTypes();
        for(index_t particleType=1; particleType<=nparticleTypes; ++particleType) {
            std::string particleTypeName = cfxExportGetParticleName(particleType);
            index_t nparticleVarCount = cfxExportGetParticleTypeVarCount(particleType);
            for(index_t particleVarNum=1; particleVarNum<=nparticleVarCount; ++particleVarNum) {
                const char *VariableName = cfxExportGetParticleTypeVarName(particleType,particleVarNum,ReadCFX::alias);
                m_allParticle.push_back(Particle(particleTypeName,VariableName,cfxExportGetParticleTypeVarDimension(particleType,particleVarNum),particleVarNum,i));
                m_numberOfParticles++;
            }
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
//        std::cerr << "m_allParam[" << i << "].varName = " << m_allParam[i].varName << std::endl;
//        std::cerr << "m_allParam[" << i << "].Dimension = " << m_allParam[i].varDimension << std::endl;
//        std::cerr << "m_allParam[" << i << "].onlyMeaningful = " << m_allParam[i].onlyMeaningfulOnBoundary << std::endl;
//        for(index_t j=0;j<m_allParam[i].vectorIdwithZone.size();++j) {
//            std::cerr << "m_allParam[" << i << "].IdwZone.ID = " << m_allParam[i].vectorIdwithZone[j].ID << std::endl;
//            std::cerr << "m_allParam[" << i << "].IdwZone.zoneFlag = " << m_allParam[i].vectorIdwithZone[j].zoneFlag << std::endl;
//        }
//    }
//    for(index_t i=0;i<m_allParticle.size();++i) {
//        std::cerr << "m_allParticle[" << i << "].particleType = " << m_allParticle[i].particleType << std::endl;
//        std::cerr << "m_allParticle[" << i << "].varName = " << m_allParticle[i].varName << std::endl;
//        std::cerr << "m_allParticle[" << i << "].Dimension = " << m_allParticle[i].varDimension << std::endl;
//        std::cerr << "m_allParticle[" << i << "].IdwZone.ID = " << m_allParticle[i].idWithZone.ID << std::endl;
//        std::cerr << "m_allParticle[" << i << "].IdwZone.zoneFlag = " << m_allParticle[i].idWithZone.zoneFlag << std::endl;
//    }
    //    for(index_t i=0;i<m_allBoundaries.size();++i) {
//        std::cerr << "m_allBoundaries[" << i << "].boundName = " << m_allBoundaries[i].boundName << std::endl;
//        std::cerr << "m_allBoundaries[" << i << "].IdwZone.ID = " << m_allBoundaries[i].idWithZone.ID << std::endl;
//        std::cerr << "m_allBoundaries[" << i << "].IdwZone.zoneFlag = " << m_allBoundaries[i].idWithZone.zoneFlag << std::endl;
//    }
//    for(index_t i=0;i<m_allRegions.size();++i) {
//        std::cerr << "m_allRegions[" << i << "].regionName = " << m_allRegions[i].regionName << std::endl;
//        std::cerr << "m_allRegions[" << i << "].IdwZone.ID = " << m_allRegions[i].idWithZone.ID << std::endl;
//        std::cerr << "m_allRegions[" << i << "].IdwZone.zoneFlag = " << m_allRegions[i].idWithZone.zoneFlag << std::endl;
//    }

    return;
}

void CaseInfo::getFieldList() {
    //field list for 3D data gets only variables that have meaningful values in the field
    //the field list for the 2D area (boundary) has all available variables

    m_boundary_param.clear();
    m_boundary_param.push_back("(NONE)");
    m_field_param.clear();
    m_field_param.push_back("(NONE)");
    m_particle_types.clear();
    m_particle_types.push_back("(NONE)");

    for(index_t varnum=0;varnum<m_numberOfVariables;++varnum) {
        if(m_allParam[varnum].onlyMeaningfulOnBoundary != 1) {
            m_field_param.push_back(m_allParam[varnum].varName);
        }
        m_boundary_param.push_back(m_allParam[varnum].varName);
    }
    for(index_t particleVarNum=0; particleVarNum<m_numberOfParticles; ++particleVarNum) {
        if(!strcmp((m_allParticle[particleVarNum].particleType).c_str(),(m_allParticle[0].particleType).c_str())) {
            std::string varName = m_allParticle[particleVarNum].varName;
            varName.pop_back(); //because each variable name has the typenumber at the end; in the list a general specifier is listed
            m_particle_types.push_back(varName);
        }
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

std::vector<Particle> CaseInfo::getCopyOfAllParticles() {
    return m_allParticle;
}

std::vector<std::string> CaseInfo::getCopyOfTrnVars() {
    return m_variableInTransientFile;
}

index_t CaseInfo::getNumberOfBoundaries() {
    return m_numberOfBoundaries;
}

index_t CaseInfo::getNumberOfRegions() {
    return m_numberOfRegions;
}

int ReadCFX::rankForVolumeAndTimestep(int setMetaTimestep, int volume, int numVolumes) const {
    //returns a rank between 0 and size(). ranks are continually distributed to processors over timestep and volumes

    int processor;
    processor = setMetaTimestep*numVolumes+volume;

    return processor % size();
}

int ReadCFX::rankFor2dAreaAndTimestep(int setMetaTimestep, int area2d, int num2dAreas) const {
    //returns a rank between 0 and size(). ranks are continually distributed to processors over timestep and volumes

    int processor;
    processor = setMetaTimestep*num2dAreas+area2d;

    return processor % size();
}

int ReadCFX::trackStartandEndForRank(int rank, int *firstTrackForRank, int *lastTrackForRank, int numberOfTracks) {
    //function evenly districutes the tracks of a particle type to the ranks and returnes the number of blocks

    int tracksPerRank = std::round(numberOfTracks/size());
    if(tracksPerRank == 0) {
        if(rank == (size()-1)) {
            *firstTrackForRank = 1;
            *lastTrackForRank = numberOfTracks;
        }
        else {
            *firstTrackForRank = 0;
            *lastTrackForRank = 0;
        }
        return 1;
    }
    else {
        if(rank == 0) {
            *firstTrackForRank = 1;
        }
        else {
            *firstTrackForRank = rank*(tracksPerRank+1)+1;
        }

        if(rank == (size()-1)) {
            *lastTrackForRank = numberOfTracks;
        }
        else {
            *lastTrackForRank = *firstTrackForRank + tracksPerRank;
        }
    }

    return (size()+1);
}

bool ReadCFX::initializeResultfile() {
    //call cfxExportInit again in order to access data out of resfile again

    std::string c = m_resultfiledir->getValue();
    const char *resultfiledir;
    resultfiledir = c.c_str();

    char *resultfileName = strdup(resultfiledir);
    m_nzones = cfxExportInit(resultfileName, counts);
    m_nnodes = counts[cfxCNT_NODE];
//    m_nelems = counts[cfxCNT_ELEMENT];
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
    //set some initialization variables if another resfile is loaded

    static double startchangeParameter = vistle::Clock::time();
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
                m_case.checkWhichVariablesAreInTransientFile(m_ntimesteps);
                initializeResultfile();
                m_case.getFieldList();
                for (auto out: m_fieldOut) {
                    setParameterChoices(out, m_case.m_field_param);
                }
                for (auto out: m_2dOut) {
                    setParameterChoices(out, m_case.m_boundary_param);
                }
                for (auto out: m_particleOut) {
                    setParameterChoices(out, m_case.m_particle_types);
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
                        sendInfo("boundary no. %d: %s",i,(allBoundaries[i-1].boundName).c_str());
                    }
                    std::vector<Region> allRegions = m_case.getCopyOfAllRegions();
                    for(index_t i=(m_case.getNumberOfBoundaries()+1);i<=(m_case.getNumberOfBoundaries()+m_case.getNumberOfRegions());++i) {
                        sendInfo("region no. %d: %s",i,(allRegions[i-m_case.getNumberOfBoundaries()-1].regionName).c_str());
                    }
                    //print variable names in .trn file
                    std::vector<std::string> trnVars = m_case.getCopyOfTrnVars();
                    sendInfo("Found %d variables in transient files", (int)trnVars.size());
                    int j=1;
                    for(std::vector<std::string>::iterator it = trnVars.begin(); it != trnVars.end(); ++it, ++j) {
                        sendInfo("%d. %s",j,(*it).c_str());
                    }
                    //print particle type names
                    index_t nParticleTypes = cfxExportGetNumberOfParticleTypes();
                    for(index_t i=1;i<=nParticleTypes;++i) {
                        sendInfo("particle type no. %d: %s",i,cfxExportGetParticleName(i));
                    }

                }
                cfxExportZoneFree();

                static double endchangeParameter = vistle::Clock::time();
                std::cerr << "Time change parameter = " << endchangeParameter-startchangeParameter << std::endl;
#ifdef CFX_DEBUG
                sendInfo("The initialisation was successfully done");
#endif
            }
        }
    }
    return Module::changeParameter(p);
}

UnstructuredGrid::ptr ReadCFX::loadGrid(int area3d) {
    //load an unstructured grid with connectivity, element and coordinate list. Each area3d gets an own unstructured grid.
    index_t nelmsIn3dArea, nconnectivities, nnodesIn3dArea;
    if(cfxExportZoneSet(m_3dAreasSelected[area3d].zoneFlag,counts) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    cfxExportZoneMotionAction(m_3dAreasSelected[area3d].zoneFlag,cfxMOTION_IGNORE);
#ifdef PARALLEL_ZONES
    nnodesIn3dArea = cfxExportNodeCount();
    nelmsIn3dArea = cfxExportElementCount();
#else
    nnodesIn3dArea = cfxExportVolumeSize(m_3dAreasSelected[area3d].ID,cfxVOL_NODES);
    nelmsIn3dArea = cfxExportVolumeSize(m_3dAreasSelected[area3d].ID,cfxVOL_ELEMS);
#endif
//    std::cerr << "m_3dAreasSelected[area3d].ID = " << m_3dAreasSelected[area3d].ID << std::endl;
//    std::cerr << "m_3dAreasSelected[area3d].zoneFlag = " << m_3dAreasSelected[area3d].zoneFlag << std::endl;
//    std::cerr << "nodesInVolume = " << nnodesIn3dArea << "; nodesInZone = " << cfxExportNodeCount() << std::endl;
//    std::cerr << "nelmsIn3dArea = " << nelmsIn3dArea << "; tets = " << counts[cfxCNT_TET] << ", " << "pyramid = " << counts[cfxCNT_PYR] << ", "<< "prism = " << counts[cfxCNT_WDG] << ", "<< "hex = " << counts[cfxCNT_HEX] << std::endl;
//    std::cerr << "area3d = " << area3d << std::endl;
    nconnectivities = 4*counts[cfxCNT_TET]+5*counts[cfxCNT_PYR]+6*counts[cfxCNT_WDG]+8*counts[cfxCNT_HEX];

    UnstructuredGrid::ptr grid(new UnstructuredGrid(nelmsIn3dArea, nconnectivities, nnodesIn3dArea)); //initialized with number of elements, number of connectivities, number of coordinates

    //load coords into unstructured grid
    auto ptrOnXcoords = grid->x().data();
    auto ptrOnYcoords = grid->y().data();
    auto ptrOnZcoords = grid->z().data();

#ifdef PARALLEL_ZONES
    cfxNode *nodes = cfxExportNodeList();
    for(index_t i=0;i<nnodesIn3dArea;++i) {
        ptrOnXcoords[i] = nodes[i].x;
        ptrOnYcoords[i] = nodes[i].y;
        ptrOnZcoords[i] = nodes[i].z;
    }
#else
    //boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);
    int *nodeListOfVolume = cfxExportVolumeList(m_3dAreasSelected[area3d].ID,cfxVOL_NODES); //query the nodes that define the volume
    index_t nnodesInZone = cfxExportNodeCount();
    std::vector<std::int32_t> nodeListOfVolumeVec;
    nodeListOfVolumeVec.resize(nnodesInZone+1);
    for(index_t i=0;i<nnodesIn3dArea;++i) {
        double xc, yc, zc;
        cfxExportNodeGet(nodeListOfVolume[i], &xc, &yc, &zc);   //get access to coordinates: [IN] nodeid [OUT] x,y,z
        ptrOnXcoords[i] = xc;
        ptrOnYcoords[i] = yc;
        ptrOnZcoords[i] = zc;
        nodeListOfVolumeVec[nodeListOfVolume[i]] = i;
    }
#endif
    cfxExportNodeFree();
    //Test, ob Einlesen funktioniert hat
//    std::cerr << "m_nnodes = " << m_nnodes << std::endl;
//    std::cerr << "nnodesIn3dArea = " << nnodesIn3dArea << std::endl;
//    std::cerr << "grid->getNumCoords()" << grid->getNumCoords() << std::endl;
//    for(int i=0;i<20;++i) {
//        std::cerr << "x,y,z (" << i << ") = " << grid->x().at(i) << ", " << grid->y().at(i) << ", " << grid->z().at(i) << std::endl;
//    }
//    std::cerr << "x,y,z (10)" << grid->x().at(10) << ", " << grid->y().at(10) << ", " << grid->z().at(10) << std::endl;

    //load element types, element list and connectivity list into unstructured grid
    int elemListCounter=0;
    auto ptrOnTl = grid->tl().data();
    auto ptrOnEl = grid->el().data();
    auto ptrOnCl = grid->cl().data();

#ifdef PARALLEL_ZONES
    cfxElement *elems = cfxExportElementList();
    for(index_t i=0;i<nelmsIn3dArea;++i) {
        switch(elems[i].type) {
            case 4: {
                ptrOnTl[i] = (UnstructuredGrid::TETRAHEDRON);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = elems[i].nodeid[nodesOfElm_counter]-1; //cfx starts nodeid with 1; index of coordinate list starts with 0. index coordinate list = nodeid
                }
                elemListCounter += 4;
                break;
            }
            case 5: {
                ptrOnTl[i] = (UnstructuredGrid::PYRAMID);
                ptrOnEl[i] = elemListCounter;
                for (int nodesOfElm_counter=0;nodesOfElm_counter<5;++nodesOfElm_counter) {
                    ptrOnCl[elemListCounter+nodesOfElm_counter] = elems[i].nodeid[nodesOfElm_counter]-1;
                }
                elemListCounter += 5;
                break;
            }
            case 6: {
                ptrOnTl[i] = (UnstructuredGrid::PRISM);
                ptrOnEl[i] = elemListCounter;

                // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
                ptrOnCl[elemListCounter+0] = elems[i].nodeid[3]-1;
                ptrOnCl[elemListCounter+1] = elems[i].nodeid[5]-1;
                ptrOnCl[elemListCounter+2] = elems[i].nodeid[4]-1;
                ptrOnCl[elemListCounter+3] = elems[i].nodeid[0]-1;
                ptrOnCl[elemListCounter+4] = elems[i].nodeid[2]-1;
                ptrOnCl[elemListCounter+5] = elems[i].nodeid[1]-1;
                elemListCounter += 6;
                break;
            }
            case 8: {
                ptrOnTl[i] = (UnstructuredGrid::HEXAHEDRON);
                ptrOnEl[i] = elemListCounter;

                // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
                ptrOnCl[elemListCounter+0] = elems[i].nodeid[4]-1;
                ptrOnCl[elemListCounter+1] = elems[i].nodeid[6]-1;
                ptrOnCl[elemListCounter+2] = elems[i].nodeid[7]-1;
                ptrOnCl[elemListCounter+3] = elems[i].nodeid[5]-1;
                ptrOnCl[elemListCounter+4] = elems[i].nodeid[0]-1;
                ptrOnCl[elemListCounter+5] = elems[i].nodeid[2]-1;
                ptrOnCl[elemListCounter+6] = elems[i].nodeid[3]-1;
                ptrOnCl[elemListCounter+7] = elems[i].nodeid[1]-1;

                elemListCounter += 8;
                break;
            }
            default: {
                std::cerr << "Elementtype(" << elems[i].type << "not yet implemented." << std::endl;
            }
        }
    }
#else
    //boost::shared_ptr<std::int32_t> nodesOfElm(new int[8]), elemtype(new int);
    std::int32_t nodesOfElm[8], elemtype[1];
    int *elmListOfVolume = cfxExportVolumeList(m_3dAreasSelected[area3d].ID,cfxVOL_ELEMS); //query the elements that define the volume
    for(index_t i=0;i<nelmsIn3dArea;++i) {
        cfxExportElementGet(elmListOfVolume[i],elemtype,nodesOfElm);
        switch(*elemtype) {
        case 4: {
            ptrOnTl[i] = (UnstructuredGrid::TETRAHEDRON);
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOfVolumeVec[nodesOfElm[nodesOfElm_counter]];
            }
            elemListCounter += 4;
            break;
        }
        case 5: {
            ptrOnTl[i] = (UnstructuredGrid::PYRAMID);
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<5;++nodesOfElm_counter) {
                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOfVolumeVec[nodesOfElm[nodesOfElm_counter]];
            }
            elemListCounter += 5;
            break;
        }
        case 6: {
            ptrOnTl[i] = (UnstructuredGrid::PRISM);
            ptrOnEl[i] = elemListCounter;

            // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
            ptrOnCl[elemListCounter+0] = nodeListOfVolumeVec[nodesOfElm[3]];
            ptrOnCl[elemListCounter+1] = nodeListOfVolumeVec[nodesOfElm[5]];
            ptrOnCl[elemListCounter+2] = nodeListOfVolumeVec[nodesOfElm[4]];
            ptrOnCl[elemListCounter+3] = nodeListOfVolumeVec[nodesOfElm[0]];
            ptrOnCl[elemListCounter+4] = nodeListOfVolumeVec[nodesOfElm[2]];
            ptrOnCl[elemListCounter+5] = nodeListOfVolumeVec[nodesOfElm[1]];
            elemListCounter += 6;
            break;
        }
        case 8: {
            ptrOnTl[i] = (UnstructuredGrid::HEXAHEDRON);
            ptrOnEl[i] = elemListCounter;

            // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
            ptrOnCl[elemListCounter+0] = nodeListOfVolumeVec[nodesOfElm[4]];
            ptrOnCl[elemListCounter+1] = nodeListOfVolumeVec[nodesOfElm[6]];
            ptrOnCl[elemListCounter+2] = nodeListOfVolumeVec[nodesOfElm[7]];
            ptrOnCl[elemListCounter+3] = nodeListOfVolumeVec[nodesOfElm[5]];
            ptrOnCl[elemListCounter+4] = nodeListOfVolumeVec[nodesOfElm[0]];
            ptrOnCl[elemListCounter+5] = nodeListOfVolumeVec[nodesOfElm[2]];
            ptrOnCl[elemListCounter+6] = nodeListOfVolumeVec[nodesOfElm[3]];
            ptrOnCl[elemListCounter+7] = nodeListOfVolumeVec[nodesOfElm[1]];

            elemListCounter += 8;
            break;
        }
        default: {
            std::cerr << "Elementtype(" << *elemtype << "not yet implemented." << std::endl;
        }
        }
    }
    cfxExportVolumeFree(m_3dAreasSelected[area3d].ID);
    grid->cl().resize(elemListCounter); //correct initialization; initialized with connectivities in zone but connectivities in 3dArea are less equal (<=) than connectivities in zone
#endif

    //element after last element
    ptrOnEl[nelmsIn3dArea] = elemListCounter;

    //Verification:
//    std::cerr << "tets = " << counts[cfxCNT_TET] << "; pyramids = " << counts[cfxCNT_PYR] << "; prism = " << counts[cfxCNT_WDG] << "; hexaeder = " << counts[cfxCNT_HEX] << std::endl;
//    //std::cerr <<"no. elems total = " << m_nelems << std::endl;
//    std::cerr <<"grid->getNumElements" << grid->getNumElements() << std::endl;
//    for(index_t i = nelmsIn3dArea-5;i<nelmsIn3dArea;++i) {
//        //std::cerr << "tl(" << i << ") = " << grid->tl().at(i) << std::endl;
//        std::cerr << "el(" << i << ") = " << grid->el().at(i) << std::endl;
//        for(index_t j = 0;j<1;++j) {
//            std::cerr << "cl(" << i*8+j << ") = " << grid->cl().at(i*8+j) << std::endl;
//        }
//    }

//    std::cerr << "grid->el(grid->getNumElements())" << grid->el().at(grid->getNumElements()) << std::endl;
//    std::cerr << "elemListCounter = " << elemListCounter << std::endl;
//    std::cerr << "grid->getNumCorners()" << grid->getNumCorners() << std::endl;
//    std::cerr << "grid->getNumVertices()" << grid->getNumVertices() << std::endl;
//    std::cerr << "nelmsIn3dArea = " << nelmsIn3dArea << std::endl;
//    std::cerr << "nconnectivities = " << nconnectivities << std::endl;
//    std::cerr << "nnodesIn3dArea = " << nnodesIn3dArea << std::endl;

//    for(index_t i=nelmsIn3dArea-10;i<=nelmsIn3dArea;++i) {
//        std::cerr << "ptrOnEl[" << i << "] = " << ptrOnEl[i] << std::endl;
//    }
//    for(int i=elemListCounter-10;i<=elemListCounter;++i) {
//        std::cerr << "ptrOnCl[" << i << "] = " << ptrOnCl[i] << std::endl;
//    }

    cfxExportElementFree();

    return grid;
}

Polygons::ptr ReadCFX::loadPolygon(int area2d) {
    //load polygon with connectivity, element and coordinate list. Each area2d gets an own polygon

    if(cfxExportZoneSet(m_2dAreasSelected[area2d].idWithZone.zoneFlag,counts) < 0) { //counts is a vector for statistics of the zone
        std::cerr << "invalid zone number" << std::endl;
    }
    cfxExportZoneMotionAction(m_2dAreasSelected[area2d].idWithZone.zoneFlag,cfxMOTION_IGNORE);
    int *nodeListOf2dArea;
    index_t nNodesIn2dArea, nFacesIn2dArea;

    if(!strcmp((m_2dAreasSelected[area2d].area2dType).c_str(),"boundary")) {
        nNodesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES);
        nodeListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
        nFacesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_FACES);
    }
    else if(!strcmp((m_2dAreasSelected[area2d].area2dType).c_str(),"region")) {
        nNodesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES);
        nodeListOf2dArea = cfxExportRegionList(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
        nFacesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_FACES);
    }
    else {
        std::cerr << "error in loadPolygon: no valid 2d area" << std::endl;
        return Polygons::ptr();
    }

    index_t nNodesInZone = cfxExportNodeCount();
    index_t nConnectIn2dArea = 4*nFacesIn2dArea; //maximum of conncectivities. If there are 3 vertices faces, it is corrected with resize at the end of the function

    Polygons::ptr polygon(new Polygons(nFacesIn2dArea,nConnectIn2dArea,nNodesIn2dArea)); //initialize Polygon with numFaces, numCorners, numVertices

    //load coords into polygon
    //boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);

    auto ptrOnXcoords = polygon->x().data();
    auto ptrOnYcoords = polygon->y().data();
    auto ptrOnZcoords = polygon->z().data();

    std::vector<std::int32_t> nodeListOf2dAreaVec;
    nodeListOf2dAreaVec.resize(nNodesInZone+1);

    for(index_t i=0;i<nNodesIn2dArea;++i) {
        double x_coord, y_coord, z_coord;
        cfxExportNodeGet(nodeListOf2dArea[i],&x_coord,&y_coord,&z_coord);  //get access to coordinates: [IN] nodeid [OUT] x,y,z
        ptrOnXcoords[i] = x_coord;
        ptrOnYcoords[i] = y_coord;
        ptrOnZcoords[i] = z_coord;
        nodeListOf2dAreaVec[nodeListOf2dArea[i]] = i;
    }

    //Verfification
//        std::cerr << "nNodesIn2dArea = " << nNodesIn2dArea << std::endl;
//        std::cerr << "polygon->getNumCoords()" << polygon->getNumCoords() << std::endl;
//        for(int i=0;i<100;++i) {
//            std::cerr << "x,y,z (" << i << ") = " << polygon->x().at(i) << ", " << polygon->y().at(i) << ", " << polygon->z().at(i) << std::endl;
//        }

    cfxExportNodeFree();
    free2dArea((m_2dAreasSelected[area2d].area2dType).c_str(), area2d);

    //attention: cfxExportBoundary/RegionList(cfxREG_FACES) has to be called AFTER cfxExportBoundary/RegionList(cfxREG_NODES) and free2dArea
    //if not, cfxExportBoundary/RegionList(cfxREG_NODES) gives unreasonable values for node id's
    int *faceListOf2dArea;
    if(!strcmp((m_2dAreasSelected[area2d].area2dType).c_str(),"boundary")) {
        faceListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_FACES); //query the faces that define the 2dArea
    }
    else if(!strcmp((m_2dAreasSelected[area2d].area2dType).c_str(),"region")) {
        faceListOf2dArea = cfxExportRegionList(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_FACES); //query the faces that define the 2dArea
    }
    else {
        std::cerr << "error in loadPolygon: no valid 2D area" << std::endl;
        int dmy = 0;
        faceListOf2dArea = &dmy;
    }

    //load face types, element list and connectivity list into polygon
    int elemListCounter=0;
    //boost::shared_ptr<std::int32_t> nodesOfFace(new int[4]);
    std::int32_t nodesOfFace[4];
    auto ptrOnEl = polygon->el().data();
    auto ptrOnCl = polygon->cl().data();

    for(index_t i=0;i<nFacesIn2dArea;++i) {
        int NumOfVerticesDefiningFace = cfxExportFaceNodes(faceListOf2dArea[i],nodesOfFace);
        assert(NumOfVerticesDefiningFace<=4); //see CFX Reference Guide p.57
        switch(NumOfVerticesDefiningFace) {
        case 3: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<3;++nodesOfElm_counter) {
                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOf2dAreaVec[nodesOfFace[nodesOfElm_counter]];
            }
            elemListCounter += 3;
            break;
        }
        case 4: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter=0;nodesOfElm_counter<4;++nodesOfElm_counter) {
                ptrOnCl[elemListCounter+nodesOfElm_counter] = nodeListOf2dAreaVec[nodesOfFace[nodesOfElm_counter]];
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
//                std::cerr << "nodesOfFace[" << j << "] = " << nodesOfFace[j] << std::endl;
//            }
//            std::cerr << "local face number[" << faceListOf2dArea[i] << "] = " << cfxFACENUM(faceListOf2dArea[i]) << std::endl;
//            std::cerr << "local element number[" << faceListOf2dArea[i] << "] = " << cfxELEMNUM(faceListOf2dArea[i]) << std::endl;
//            std::cerr << "size of nodesOfFace = " << NumOfVerticesDefiningFace << std::endl;
//        }
    }

    //element after last element in element list and connectivity list
    polygon->cl().resize(elemListCounter);
    ptrOnEl[nFacesIn2dArea] = elemListCounter;

    //Verfification
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

    free2dArea((m_2dAreasSelected[area2d].area2dType).c_str(), area2d);

    return polygon;
}

vistle::Lines::ptr ReadCFX::loadParticleTrackCoords(int particleTypeNumber, const index_t numVertices, int firstTrackForRank, int lastTrackForRank) {
    //std::cerr << "m_particleTypesSelected[" << particleTypeNumber << "] = " << m_particleTypesSelected[particleTypeNumber] << std::endl;

    std::cerr << "numberVertices = " << numVertices << std::endl;
    Lines::ptr lines(new Lines(numVertices, numVertices, numVertices)); //element list and corner list are initialized with number of vertices, because only "dot elements" are used in the line
    auto ptrOnXCoords = lines->x().data();
    auto ptrOnYCoords = lines->y().data();
    auto ptrOnZCoords = lines->z().data();

    int pointsOnTrackCounter = 0;
    for(int i=firstTrackForRank; i<=lastTrackForRank;++i) {
        int NumOfPointsOnTrack = cfxExportGetNumberOfPointsOnParticleTrack(m_particleTypesSelected[particleTypeNumber],i);
        float *ParticleTrackCoords = cfxExportGetParticleTrackCoordinatesByTrack(m_particleTypesSelected[particleTypeNumber],i,&NumOfPointsOnTrack);
        std::cerr << "Track Nr = " << i << std::endl;
        for(int k=0;k<NumOfPointsOnTrack;++k) {
            ptrOnXCoords[pointsOnTrackCounter + k] = ParticleTrackCoords[k*3];
            ptrOnYCoords[pointsOnTrackCounter + k] = ParticleTrackCoords[k*3+1];
            ptrOnZCoords[pointsOnTrackCounter + k] = ParticleTrackCoords[k*3+2];
        }
        pointsOnTrackCounter += NumOfPointsOnTrack;
    }

    std::cerr << "pointsOnTrackCounter = " << pointsOnTrackCounter << std::endl;
    return lines;
}

DataBase::ptr ReadCFX::loadField(int area3d, Variable var) {
    //load scalar data in Vec and vector data in Vec3 for the area3d
    for(index_t i=0;i<var.vectorIdwithZone.size();++i) {
        if(var.vectorIdwithZone[i].zoneFlag == m_3dAreasSelected[area3d].zoneFlag) {
            if(cfxExportZoneSet(m_3dAreasSelected[area3d].zoneFlag,NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
            }
            cfxExportZoneMotionAction(m_3dAreasSelected[area3d].zoneFlag,cfxMOTION_IGNORE);

#ifdef PARALLEL_ZONES
            index_t nnodesInZone = cfxExportNodeCount();
            //read field parameters
            index_t varnum = var.vectorIdwithZone[i].ID;
            float *variableList = cfxExportVariableList(varnum,correct);
            if(var.varDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nnodesInZone));
                scalar_t *ptrOnScalarData = s->x().data();
                for(index_t j=0;j<nnodesInZone;++j) {
                    ptrOnScalarData[j] = variableList[j];
//                    if(j<10) {
//                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
//                    }
                }
                cfxExportVariableFree(varnum);
                return s;
            }
            else if(var.varDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nnodesInZone));
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();

                for(index_t j=0;j<nnodesInZone;++j) {
                    ptrOnVectorXData[j] = variableList[3*j];
                    ptrOnVectorYData[j] = variableList[3*j+1];
                    ptrOnVectorZData[j] = variableList[3*j+2];
//                    if(j<20) {
//                        std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
//                        std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
//                        std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
//                    }
                }
                cfxExportVariableFree(varnum);
                return v;
            }
#else
            //    std::cerr << "m_3dAreasSelected[area3d].ID = " << m_3dAreasSelected[area3d].ID << "; m_3dAreasSelected[area3d].zoneFlag = " << m_3dAreasSelected[area3d].zoneFlag << std::endl;
            index_t nnodesInVolume = cfxExportVolumeSize(m_3dAreasSelected[area3d].ID,cfxVOL_NODES);
            int *nodeListOfVolume = cfxExportVolumeList(m_3dAreasSelected[area3d].ID,cfxVOL_NODES); //query the nodes that define the volume

            //read field parameters
            index_t varnum = var.vectorIdwithZone[i].ID;

            if(var.varDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nnodesInVolume));
                scalar_t *ptrOnScalarData = s->x().data();

                for(index_t j=0;j<nnodesInVolume;++j) {
                    float value;
                    cfxExportVariableGet(varnum,correct,nodeListOfVolume[j],&value);
                    ptrOnScalarData[j] = value;
//                    if(j<10) {
//                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
//                    }
                }
                //cfxExportVariableFree(varnum);
                cfxExportVolumeFree(m_3dAreasSelected[area3d].ID);
                return s;
            }
            else if(var.varDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nnodesInVolume));
                float value[3];
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();

                for(index_t j=0;j<nnodesInVolume;++j) {
                    cfxExportVariableGet(varnum,correct,nodeListOfVolume[j],value);
                    ptrOnVectorXData[j] = value[0];
                    ptrOnVectorYData[j] = value[1];
                    ptrOnVectorZData[j] = value[2];
//                    if(j<20) {
//                        std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
//                        std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
//                        std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
//                    }
                }
                //cfxExportVariableFree(varnum);
                cfxExportVolumeFree(m_3dAreasSelected[area3d].ID);
                return v;
            }
            cfxExportVolumeFree(m_3dAreasSelected[area3d].ID);
#endif
        }
    }
    return DataBase::ptr();
}

bool ReadCFX::free2dArea(const char *area2dType, int area2d) {
    //function to call cfxExportFree... depending which kind of 2d area is read.
    //cfx API requires that after a call of cfxBoundaryGet oder cfxRegionGet the memory needs to be freed
    //area2d can either be boundaries or regions
    if(!strcmp(area2dType,"boundary")) {
        cfxExportBoundaryFree(m_2dAreasSelected[area2d].idWithZone.ID);
    }
    else if(!strcmp(area2dType,"region")) {
        cfxExportRegionFree(m_2dAreasSelected[area2d].idWithZone.ID);
    }
    else {
        std::cerr << "Fehler free2dArea" << std::endl;
    }
    return true;
}

DataBase::ptr ReadCFX::load2dField(int area2d, Variable var) {
    //load scalar data in Vec and vector data in Vec3 for the area2d
    for(index_t i=0;i<var.vectorIdwithZone.size();++i) {
        if(m_2dAreasSelected[area2d].idWithZone.zoneFlag == var.vectorIdwithZone[i].zoneFlag) {
            if(cfxExportZoneSet(m_2dAreasSelected[area2d].idWithZone.zoneFlag,NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
            }
            cfxExportZoneMotionAction(m_2dAreasSelected[area2d].idWithZone.zoneFlag,cfxMOTION_IGNORE);
            index_t nNodesIn2dArea=0;
            int *nodeListOf2dArea;
            if(!strcmp((m_2dAreasSelected[area2d].area2dType).c_str(),"boundary")) {
                nNodesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES);
                nodeListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
            }
            else if(!strcmp((m_2dAreasSelected[area2d].area2dType).c_str(),"region")) {
                nNodesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES);
                nodeListOf2dArea = cfxExportRegionList(m_2dAreasSelected[area2d].idWithZone.ID,cfxREG_NODES); //query the nodes that define the 2dArea
            }
            else {
                std::cerr << "Fehler in load2dAreaField 1" << std::endl;
            }

            //read field parameters
            index_t varnum = var.vectorIdwithZone[i].ID;
            if(var.varDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nNodesIn2dArea));
                //boost::shared_ptr<float_t> value(new float);
                scalar_t *ptrOnScalarData = s->x().data();
                for(index_t j=0;j<nNodesIn2dArea;++j) {
                    float value;
                    cfxExportVariableGet(varnum,correct,nodeListOf2dArea[j],&value);
                    ptrOnScalarData[j] = value;
//                    if(j<100) {
//                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
//                    }
                }
                //cfxExportVariableFree(varnum);
                free2dArea((m_2dAreasSelected[area2d].area2dType).c_str(), area2d);
                return s;
            }
            else if(var.varDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nNodesIn2dArea));
                //boost::shared_ptr<float_t> value(new float[3]);
                float value[3];
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();
                for(index_t j=0;j<nNodesIn2dArea;++j) {
                    cfxExportVariableGet(varnum,correct,nodeListOf2dArea[j],value);
                    ptrOnVectorXData[j] = value[0];
                    ptrOnVectorYData[j] = value[1];
                    ptrOnVectorZData[j] = value[2];
//                    if(j<100) {
//                        std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
//                        std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
//                        std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
//                    }
                }
                //cfxExportVariableFree(varnum);
                free2dArea((m_2dAreasSelected[area2d].area2dType).c_str(), area2d);
                return v;
            }
            free2dArea((m_2dAreasSelected[area2d].area2dType).c_str(), area2d);
        }
    }
    return DataBase::ptr();
}

vistle::DataBase::ptr ReadCFX::loadParticleTime(int particleTypeNumber, const index_t NumVertices, int firstTrackForRank, int lastTrackForRank) {
    // loads the time values for each point on every track of the particle type

    int pointsOnTrackCounter = 0;
    Vec<Scalar>::ptr s(new Vec<Scalar>(NumVertices));
    scalar_t *ptrOnScalarData = s->x().data();
    for(int j=firstTrackForRank; j<=lastTrackForRank; ++j) {
        int NumOfPointsOnTrack = cfxExportGetNumberOfPointsOnParticleTrack(m_particleTypesSelected[particleTypeNumber],j);
        float *ParticleTrackTime = cfxExportGetParticleTrackTimeByTrack(m_particleTypesSelected[particleTypeNumber],j,&NumOfPointsOnTrack);
        for(int k=0;k<NumOfPointsOnTrack;++k) {
            ptrOnScalarData[pointsOnTrackCounter + k] = ParticleTrackTime[k];
        }
        pointsOnTrackCounter += NumOfPointsOnTrack;
    }

    return s;
}


vistle::DataBase::ptr ReadCFX::loadParticleValues(int particleTypeNumber, Particle particle, const index_t NumVertices, int firstTrackForRank, int lastTrackForRank) {
    //loads the values for each point on every track of the given particle type and variable

    int pointsOnTrackCounter = 0;

    if(particle.varDimension == 1) {
        Vec<Scalar>::ptr s(new Vec<Scalar>(NumVertices));
        scalar_t *ptrOnScalarData = s->x().data();
        for(int j=firstTrackForRank; j<=lastTrackForRank; ++j) {
            int NumOfPointsOnTrack = cfxExportGetNumberOfPointsOnParticleTrack(m_particleTypesSelected[particleTypeNumber],j);
            float *particleVar = cfxExportGetParticleTypeVar(m_particleTypesSelected[particleTypeNumber],particle.idWithZone.ID,j);
            for(int k=0;k<NumOfPointsOnTrack;++k) {
                ptrOnScalarData[pointsOnTrackCounter + k] = particleVar[k];
            }
            pointsOnTrackCounter += NumOfPointsOnTrack;
        }
        return s;
    }
    if(particle.varDimension == 3) {
        Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(NumVertices));
        scalar_t *ptrOnVectorXData = v->x().data();
        scalar_t *ptrOnVectorYData = v->y().data();
        scalar_t *ptrOnVectorZData = v->z().data();
        for(int j=firstTrackForRank; j<=lastTrackForRank; ++j) {
            int NumOfPointsOnTrack = cfxExportGetNumberOfPointsOnParticleTrack(m_particleTypesSelected[particleTypeNumber],j);
            float *particleVar = cfxExportGetParticleTypeVar(m_particleTypesSelected[particleTypeNumber],particle.idWithZone.ID,j);
            for(int k=0;k<NumOfPointsOnTrack;++k) {
                ptrOnVectorXData[pointsOnTrackCounter + k] = particleVar[3*k];
                ptrOnVectorYData[pointsOnTrackCounter + k] = particleVar[3*k+1];
                ptrOnVectorZData[pointsOnTrackCounter + k] = particleVar[3*k+2];
            }
            pointsOnTrackCounter += NumOfPointsOnTrack;
        }
        return v;
    }
    return DataBase::ptr();
}

index_t ReadCFX::collect3dAreas() {
    //write either selected zones or volumes (in selected zones) in a vector in a consecutively order

    // read zone selection; m_coRestraintZones contains a bool array of selected zones
    //m_coRestraintZones(zone) = 1 zone is selected
    //m_coRestraintZones(zone) = 0 zone isn't selected
    //group = -1 zones are selected
    m_coRestraintZones.clear();
    m_coRestraintZones.add(m_zoneSelection->getValue());
    ssize_t val = m_coRestraintZones.getNumGroups(), group;
    m_coRestraintZones.get(val,group);

    index_t numberOfSelected3dAreas=0;
    m_3dAreasSelected.clear();
#ifdef PARALLEL_ZONES
    m_3dAreasSelected.resize(m_nzones);
#else
    m_3dAreasSelected.resize(m_nvolumes);
#endif
    for(index_t i=1;i<=m_nzones;++i) {
        if(m_coRestraintZones(i)) {
#ifdef PARALLEL_ZONES
            m_3dAreasSelected[numberOfSelected3dAreas]=IdWithZoneFlag(0,i);
            numberOfSelected3dAreas++;
#else
            if(cfxExportZoneSet(i,NULL)<0) {
                std::cerr << "invalid zone number" << std::endl;
                return numberOfSelected3dAreas;
            }
            int nvolumes = cfxExportVolumeCount();
            for(int j=1;j<=nvolumes;++j) {
                //std::cerr << "volumeName no. " << j << " in zone. " << i << " = " << cfxExportVolumeName(j) << std::endl;
                m_3dAreasSelected[numberOfSelected3dAreas]=IdWithZoneFlag(j,i);
                numberOfSelected3dAreas++;
            }
#endif
        }
    }

#ifndef PARALLEL_ZONES
    m_3dAreasSelected.resize(numberOfSelected3dAreas);
#endif


    //zum Testen
//    for(index_t i=0;i<numberOfSelected3dAreas;++i) {
//        std::cerr << "m_3dAreasSelected[" << i << "].ID = " << m_3dAreasSelected[i].ID << " m_3dAreasSelected.zoneFlag" << m_3dAreasSelected[i].zoneFlag << std::endl;
//    }
    cfxExportZoneFree();
    return numberOfSelected3dAreas;
}

index_t ReadCFX::collect2dAreas() {
    //write either selected boundaries and regions in a vector in a consecutively order

    //m_coRestraint2dAreas contains a bool array of selected 2dAreas
    //m_coRestraint2dAreas(Area2d) = 1 Area2d is selected
    //m_coRestraint2dAreas(Area2d) = 0 Area2d isn't selected
    //group = -1 all Area2d are selected
    m_coRestraint2dAreas.clear();
    m_coRestraint2dAreas.add(m_2dAreaSelection->getValue());
    ssize_t val = m_coRestraint2dAreas.getNumGroups(), group;
    m_coRestraint2dAreas.get(val,group);

    index_t numberOfSelected2dAreas=0;
    m_2dAreasSelected.clear();
    m_2dAreasSelected.resize(m_case.getNumberOfBoundaries()+m_case.getNumberOfRegions());
    std::vector<Boundary> allBoundaries = m_case.getCopyOfAllBoundaries();
    std::vector<Region> allRegions = m_case.getCopyOfAllRegions();

    for(index_t i=1;i<=m_case.getNumberOfBoundaries();++i) {
        if(m_coRestraint2dAreas(i)) {
            m_2dAreasSelected[numberOfSelected2dAreas].idWithZone=IdWithZoneFlag(allBoundaries[i-1].idWithZone.ID,allBoundaries[i-1].idWithZone.zoneFlag);
            m_2dAreasSelected[numberOfSelected2dAreas].area2dType = "boundary";
            numberOfSelected2dAreas++;
        }
    }
    for(index_t i=m_case.getNumberOfBoundaries()+1;i<=(m_case.getNumberOfBoundaries()+m_case.getNumberOfRegions());++i) {
        if(m_coRestraint2dAreas(i)) {
            m_2dAreasSelected[numberOfSelected2dAreas].idWithZone=IdWithZoneFlag(allRegions[i-m_case.getNumberOfBoundaries()-1].idWithZone.ID,allRegions[i-m_case.getNumberOfBoundaries()-1].idWithZone.zoneFlag);
            m_2dAreasSelected[numberOfSelected2dAreas].area2dType = "region";
            numberOfSelected2dAreas++;
        }
    }

    m_2dAreasSelected.resize(numberOfSelected2dAreas);

    //Verification
//    if(rank()==0) {
//        for(index_t i=0;i<numberOfSelected2dAreas;++i) {
//            std::cerr << "m_2dAreasSelected[" << i << "] = " << m_2dAreasSelected[i].idWithZone.ID << "; zoneflag = " << m_2dAreasSelected[i].idWithZone.zoneFlag << std::endl;
//        }
//    }

    cfxExportZoneFree();
    return numberOfSelected2dAreas;
}

index_t ReadCFX::collectParticles() {
    //write selected particle types in a vector in consecutively order
    //m_coRestraintParticle contains a bool array of selected Particles
    //m_coRestraintParticle(Area2d) = 1 Particle is selected
    //m_coRestraintParticle(Area2d) = 0 Particle isn't selected
    //group = -1 all particles are selected
    m_coRestraintParticle.clear();
    m_coRestraintParticle.add(m_particleSelection->getValue());
    ssize_t val = m_coRestraintParticle.getNumGroups(), group;
    m_coRestraintParticle.get(val,group);

    index_t numberOfSelectedParticles=0;
    m_particleTypesSelected.clear();
    index_t nParticleTypes = cfxExportGetNumberOfParticleTypes();
    m_particleTypesSelected.resize(nParticleTypes);

    for(index_t i=1;i<=nParticleTypes;++i) {
        if(m_coRestraintParticle(i)) {
            m_particleTypesSelected[numberOfSelectedParticles]=i;
            numberOfSelectedParticles++;
        }
    }
    m_particleTypesSelected.resize(numberOfSelectedParticles);

    //Verification
//    if(rank()==0) {
//        for(index_t i=0;i<numberOfSelectedParticles;++i) {
//            std::cerr << "m_particleTypesSelected[" << i << "] = " << m_particleTypesSelected[i] << std::endl;
//        }
//    }

    return numberOfSelectedParticles;
}


bool ReadCFX::loadFields(int area3d, int setMetaTimestep, int timestep, int numTimesteps, index_t numSel3dArea, bool readTransientFile) {
    //calles for each port the loadGrid and loadField function and the setDataObject function to get the object ready to be added to port

    for (int i=0; i<NumPorts; ++i) {
      std::string field = m_fieldOut[i]->getValue();
      std::vector<Variable> allParam = m_case.getCopyOfAllParam();
      std::vector<std::string> trnVars = m_case.getCopyOfTrnVars();
      auto it = find_if(allParam.begin(), allParam.end(), [&field](const Variable& obj) {
          return obj.varName == field;});
      if (it == allParam.end()) {
              m_currentVolumedata[i]= DataBase::ptr();
      }
      else {
          auto index = std::distance(allParam.begin(), it);
          DataBase::ptr obj = loadField(area3d, allParam[index]);

          if(std::find(trnVars.begin(), trnVars.end(), allParam[index].varName) == trnVars.end()) {
              //variable exists only in resfile --> timestep = -1
              setDataObject(m_gridsInTimestep[area3d],obj,area3d,setMetaTimestep,-1,numTimesteps,numSel3dArea,readTransientFile);
          }
          else {
              //variable exists in resfile and in transient files --> timestep = last
              setDataObject(m_gridsInTimestep[area3d],obj,area3d,setMetaTimestep,timestep,numTimesteps,numSel3dArea,readTransientFile);
          }
          m_currentVolumedata[i]= obj;
      }
   }
   return true;
}

bool ReadCFX::load2dFields(int area2d, int setMetaTimestep, int timestep, int numTimesteps, index_t numSel2dArea, bool readTransientFile) {
    //calles for each port the loadPolygon and load2dField function and the set2dDataObject function to get the object ready to be added to port

    for (int i=0; i<Num2dPorts; ++i) {
        std::string area2dField = m_2dOut[i]->getValue();
        std::vector<Variable> allParam = m_case.getCopyOfAllParam();
        std::vector<std::string> trnVars = m_case.getCopyOfTrnVars();
        auto it = find_if(allParam.begin(), allParam.end(), [&area2dField](const Variable& obj) {
            return obj.varName == area2dField;});
        if (it == allParam.end()) {
            m_current2dData[i] = DataBase::ptr();
        }
        else {
            auto index = std::distance(allParam.begin(), it);
            DataBase::ptr obj = load2dField(area2d, allParam[index]);

            if(std::find(trnVars.begin(), trnVars.end(), allParam[index].varName) == trnVars.end()) {
                //variable exists only in resfile --> timestep = -1
                set2dObject(m_polygonsInTimestep[area2d],obj,area2d,setMetaTimestep,-1,numTimesteps,numSel2dArea,readTransientFile);
            }
            else {
                //variable exists in resfile and in transient files --> timestep = last
                set2dObject(m_polygonsInTimestep[area2d],obj,area2d,setMetaTimestep,timestep,numTimesteps,numSel2dArea,readTransientFile);

            }
            m_current2dData[i]= obj;
        }
    }
    return true;
}

bool ReadCFX::loadParticles(int particleTypeNumber) {
    //calles loadParticleTrackCoords, loads the time of the particles and calles loadParticleValues for each variable, sets the coords to the values and calles addParticlesToPort

    std::vector<Particle> allParticle = m_case.getCopyOfAllParticles();
    if(cfxExportZoneSet(allParticle[0].idWithZone.zoneFlag,NULL) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    cfxExportZoneMotionAction(allParticle[0].idWithZone.zoneFlag,cfxMOTION_IGNORE);
    int numberOfTracks = cfxExportGetNumberOfTracks(m_particleTypesSelected[particleTypeNumber]);

    int firstTrackForRank, lastTrackForRank;
    int numberOfBlocks = trackStartandEndForRank(rank(),&firstTrackForRank,&lastTrackForRank,numberOfTracks);

    std::cerr << "numberofTrack = " << numberOfTracks << std::endl;
    std::cerr << "rank = (" << rank() << "/" << size() << "); first = " << firstTrackForRank << "; last = " << lastTrackForRank << std::endl;


    index_t NumVertices = 0;
    for(int i=firstTrackForRank; i<=lastTrackForRank;++i) {
        NumVertices += cfxExportGetNumberOfPointsOnParticleTrack(m_particleTypesSelected[particleTypeNumber],i);
    }
    m_coordsOfParticles = loadParticleTrackCoords(particleTypeNumber, NumVertices, firstTrackForRank, lastTrackForRank);
    m_currentParticleData[0] = loadParticleTime(particleTypeNumber, NumVertices, firstTrackForRank, lastTrackForRank);

    m_coordsOfParticles->setNumBlocks(numberOfBlocks);
    m_currentParticleData[0]->setNumBlocks(numberOfBlocks);
    if(numberOfBlocks > 1) {
        m_coordsOfParticles->setBlock(rank()+1);
        m_currentParticleData[0]->setBlock(rank()+1);
    }
    else {
        m_coordsOfParticles->setBlock(1);
        m_currentParticleData[0]->setBlock(1);
    }
    m_currentParticleData[0]->setGrid(m_coordsOfParticles);

    for (int i=0; i<NumParticlePorts; ++i) {
        std::string field = m_particleOut[i]->getValue();
        std::string s=std::to_string(particleTypeNumber);
        field.append(s.c_str());
        std::cerr << "field = " << field << std::endl;

        auto it = find_if(allParticle.begin(), allParticle.end(), [&field](const Particle& obj) {
            return obj.varName == field;});
        if (it == allParticle.end()) {
            m_currentParticleData[i]= DataBase::ptr();
        }
        else {
            auto index = std::distance(allParticle.begin(), it);
            m_currentParticleData[i+1] = loadParticleValues(particleTypeNumber, allParticle[index], NumVertices, firstTrackForRank, lastTrackForRank);
        }
        if(m_currentParticleData[i+1]) {
            m_currentParticleData[i+1]->setNumBlocks(numberOfBlocks);
            if(numberOfBlocks > 1) {
                m_coordsOfParticles->setBlock(rank()+1);
                m_currentParticleData[i+1]->setBlock(rank()+1);
            }
            else {
                m_coordsOfParticles->setBlock(1);
                m_currentParticleData[i+1]->setBlock(1);
            }
            m_currentParticleData[i+1]->setGrid(m_coordsOfParticles);
        }
    }
    addParticleToPorts();

    return true;
}

bool ReadCFX::addVolumeDataToPorts() {
    //adds the 3d data to ports

    for (int portnum=0; portnum<NumPorts; ++portnum) {
        if(m_currentVolumedata[portnum]) {
            addObject(m_volumeDataOut[portnum], m_currentVolumedata[portnum]);
        }
    }
    return true;
}

bool ReadCFX::add2dDataToPorts() {
    //adds the 2d data to ports

    for (int portnum=0; portnum<Num2dPorts; ++portnum) {
        if(m_current2dData[portnum]) {
            addObject(m_2dDataOut[portnum], m_current2dData[portnum]);
        }
    }
    return true;
}

bool ReadCFX::addGridToPort(int area3d) {
    //adds the grid to the gridOut port

    if(m_gridsInTimestep[area3d]) {
        addObject(m_gridOut,m_gridsInTimestep[area3d]);
    }
    return true;
}

bool ReadCFX::addPolygonToPort(int area2d) {
    //adds the polygon to the polygonOut port

    if(m_polygonsInTimestep[area2d]) {
        addObject(m_polyOut,m_polygonsInTimestep[area2d]);
    }
    return true;
}

bool ReadCFX::addParticleToPorts() {
    //adds particles to port: one time port and NumParticlePorts data ports

    for (int portnum=0; portnum<=NumParticlePorts; ++portnum) {
        if(m_currentParticleData[portnum]) {
            if(portnum == 0) {
                addObject(m_particleTime, m_currentParticleData[portnum]);
            }
            else {
                addObject(m_particleDataOut[portnum], m_currentParticleData[portnum-1]);
            }
        }
    }
    return true;
}

void ReadCFX::setMeta(Object::ptr obj, int blockNr, int setMetaTimestep, int timestep, int numTimesteps, index_t totalBlockNr, bool readTransientFile) {
    //sets the timestep, the number of timesteps, the real time, the block number and total number of blocks and the transformation matrix for a vistle object
    //timestep is -1 if ntimesteps is 0 or a variable is only in resfile
    //timestep for data in .trn files is timestep (variable)
    //timestep for data in .res files is last timestep (fix)

    if (obj) {
       if(m_ntimesteps == 0) {
           obj->setTimestep(-1);
           obj->setNumTimesteps(-1);
           obj->setRealTime(0);
       }
       else {
           if(timestep == -1 || (m_lasttimestep->getValue() == 0)) {
               obj->setNumTimesteps(-1);
               obj->setTimestep(-1);
           }
           else {
               if(readTransientFile) {   //readTransientFile == 1 --> data belong to transient file
                   obj->setRealTime(cfxExportTimestepTimeGet(timestep+1)); //+1 because cfxExport API's start with Index 1
//                   std::cerr << "NumTimesteps = " << numTimesteps << std::endl;
//                   std::cerr << "setMetaTimestep = " << setMetaTimestep << std::endl;
               }
               else {
                   obj->setRealTime(cfxExportTimestepTimeGet(m_lasttimestep->getValue()+1)+cfxExportTimestepTimeGet(1));  //+1 because cfxExport API's start with Index 1
               }
               obj->setNumTimesteps(numTimesteps);
               std::cerr << "rank = " << rank() << "; setMetaTimestep = " << setMetaTimestep << std::endl;
               obj->setTimestep(setMetaTimestep);
           }
       }
      obj->setBlock(blockNr);
      obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);


      double rotAxis[2][3] = {{0.0, 0.0, 0.0},{0.0, 0.0, 0.0}};
      double angularVel; //in radians per second
      if(cfxExportZoneIsRotating(rotAxis,&angularVel)) { //1 if zone is rotating, 0 if zone is not rotating

 //          for(int i=0;i<3;++i) {
 //              std::cerr << "rotAxis[0][" << i << "] = " << rotAxis[0][i] << std::endl;
 //          }
 //          for(int i=0;i<3;++i) {
 //              std::cerr << "rotAxis[1][" << i << "] = " << rotAxis[1][i] << std::endl;
 //          }

          double startTime = cfxExportTimestepTimeGet(1);
          double currentTime = cfxExportTimestepTimeGet(timestep+1);
          Scalar rot_angle = fmod((angularVel*(currentTime-startTime)),(2*M_PI)); //in radians

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

bool ReadCFX::setDataObject(UnstructuredGrid::ptr grid, DataBase::ptr data, int area3d, int setMetaTimestep, int timestep, int numTimesteps, index_t numSel3dArea, bool readTransientFile) {
    //function guarantees that each vistle object gets all necessary meta information (timestep, number of timestep, ...)

    setMeta(grid,area3d,setMetaTimestep,timestep,numTimesteps,numSel3dArea,readTransientFile);
    setMeta(data,area3d,setMetaTimestep,timestep,numTimesteps,numSel3dArea,readTransientFile);
    data->setGrid(grid);
    data->setMapping(DataBase::Vertex);

    return true;
}

bool ReadCFX::set2dObject(Polygons::ptr polygon, DataBase::ptr data, int area2d, int setMetaTimestep, int timestep, int numTimesteps, index_t numSel2dArea, bool readTransientFile) {
    //function guarantees that each vistle object gets all necessary meta information (timestep, number of timestep, ...)

    setMeta(polygon,area2d,setMetaTimestep,timestep,numTimesteps,numSel2dArea,readTransientFile);
    setMeta(data,area2d,setMetaTimestep,timestep,numTimesteps,numSel2dArea,readTransientFile);
    data->setGrid(polygon);
    data->setMapping(DataBase::Vertex);

    return true;
}

bool ReadCFX::readTime(index_t numSel3dArea, index_t numSel2dArea, int setMetaTimestep, int timestep, int numTimesteps, bool readTransientFile) {
    //reads all information (3d and 2d data) for a given timestep and adds them to the ports

    for(index_t i=0;i<numSel3dArea;++i) {
        //std::cerr << "rankForVolumeAndTimestep(" << timestep << "," << i << "," << numSel3dArea << ") = " << rankForVolumeAndTimestep(timestep,i,numSel3dArea) << std::endl;
        if(rankForVolumeAndTimestep(setMetaTimestep,i,numSel3dArea) == rank()) {
            std::cerr << "process mit rank() = " << rank() << "; berechnet volume = " << i << "; in setMetaTimestep = " << setMetaTimestep << std::endl;

            if(!m_gridsInTimestep[i] || cfxExportGridChanged(m_previousTimestep,timestep+1)) {
                m_gridsInTimestep[i] = loadGrid(i);
            }
            //setMeta(m_gridsInTimestep[i],i,setMetaTimestep,timestep,numSel3dArea,readTransientFile);
            addGridToPort(i);
            loadFields(i, setMetaTimestep, timestep, numTimesteps, numSel3dArea, readTransientFile);
            addVolumeDataToPorts();
        }
    }
    for(index_t i=0;i<numSel2dArea;++i) {
        if(rankFor2dAreaAndTimestep(setMetaTimestep,i,numSel2dArea) == rank()) {
//            std::cerr << "process mit rank() = " << rank() << "; berechnet area2d = " << i << "; in setMetaTimestep = " << setMetaTimestep << std::endl;
            if(!m_polygonsInTimestep[i] || cfxExportGridChanged(m_previousTimestep,timestep+1)) {
                m_polygonsInTimestep[i] = loadPolygon(i);
            }
            addPolygonToPort(i);
            load2dFields(i,setMetaTimestep, timestep, numTimesteps, numSel2dArea, readTransientFile);
            add2dDataToPorts();
        }
    }
    m_previousTimestep = timestep+1;
    return true;
}


bool ReadCFX::compute() {
    //starts reading the .res file and loops than over all timesteps
#ifdef CFX_DEBUG
    std::cerr << "Compute Start. \n";
#endif

    index_t setMetaTimestep=0;
    index_t firsttimestep=m_firsttimestep->getValue(), lasttimestep=m_lasttimestep->getValue(), timeskip=m_timeskip->getValue();
    index_t numTimesteps = ((lasttimestep-firsttimestep)/(timeskip+1))+2;
    bool readTransientFile;

    if(!m_case.m_valid) {
        sendInfo("not a valid .res file entered: initialisation not yet done");
    }
    else {
        if(m_ExportDone) {
            static double startInitialize = vistle::Clock::time();

            initializeResultfile();
            m_case.parseResultfile();

            static double endInitialize = vistle::Clock::time();
            std::cerr << "Time Initialize = " << endInitialize-startInitialize << std::endl;
        }
        //read variables out of .res file
        static double startRestCompute = vistle::Clock::time();

        readTransientFile = 0;
        index_t numSel3dArea = collect3dAreas();
        index_t numSel2dArea = collect2dAreas();
        index_t numSelParticleTypes = collectParticles();
        m_gridsInTimestep.resize(numSel3dArea);
        m_polygonsInTimestep.resize(numSel2dArea);
        readTime(numSel3dArea,numSel2dArea,numTimesteps-1,0,numTimesteps,readTransientFile);

        //read particles
        if(numSelParticleTypes>0) {
            for(index_t i=0;i<numSelParticleTypes;++i) {
                loadParticles(i);
            }
        }

        //read variables out of timesteps .trn file
        if(m_ntimesteps!=0 && (lasttimestep!=0)) {
            readTransientFile = 1;
            for(index_t timestep = firsttimestep; timestep<=lasttimestep; timestep+=(timeskip+1)) {
                index_t timestepNumber = cfxExportTimestepNumGet(timestep+1);
                //cfxExportTimestepTimeGet(timestepNumber);
                if(cfxExportTimestepSet(timestepNumber)<0) {
                    std::cerr << "cfxExportTimestepSet: invalid timestep number(" << timestepNumber << ")" << std::endl;
                }
                else {
                    m_case.parseResultfile();
                    numSel3dArea = collect3dAreas();
                    numSel2dArea = collect2dAreas();
                    readTime(numSel3dArea,numSel2dArea,setMetaTimestep,timestep,numTimesteps,readTransientFile);
                }
                setMetaTimestep++;
            }
            cfxExportDone();
            m_ExportDone = true;
        }
        static double endRestCompute = vistle::Clock::time();
        std::cerr << "Time Rest of Compute = " << endRestCompute-startRestCompute << std::endl;
    }

// #####################################################################################################################################
    // ATTENTION: Memorymanagement must be fixed!!!
    // ReadCFX crashes when all vectors are cleared and ReadCFX is executed twice
    // gridsInTimestep is uncommented because if all grids are stored there can't be made a zone selection anymore
// #####################################################################################################################################
//    m_fieldOut.clear();
//    m_2dOut.clear();
//    m_particleOut.clear();
//    m_volumeDataOut.clear();
//    m_2dDataOut.clear();
//    m_particleDataOut.clear();
      m_gridsInTimestep.clear();
      m_polygonsInTimestep.clear();
//    m_coordsOfParticles.reset();
//    m_currentVolumedata.clear();
//    m_current2dData.clear();
//    m_currentParticleData.clear();
//    grid.reset();



    return true;
}
