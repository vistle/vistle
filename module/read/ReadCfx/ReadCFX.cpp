#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <mutex>


#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/normals.h>
#include <vistle/core/unstr.h>
#include <vistle/core/lines.h>

#include <vistle/util/coRestraint.h>

// Includes for the CFX application programming interface (API)
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cfxExport.h> // linked but still qtcreator doesn't find it
#include <getargs.h> // linked but still qtcreator doesn't find it
#include <iostream>
//#include <unistd.h>

#include <boost/format.hpp>
#include <boost/cstdint.hpp>

#include <vistle/util/stopwatch.h>
#include <vistle/util/filesystem.h>


#include "ReadCFX.h"

//#define CFX_DEBUG
#define PARALLEL_ZONES


namespace fs = vistle::filesystem;

MODULE_MAIN(ReadCFX)

namespace {
std::mutex cfxMutex;
vistle::Module *cfxModule = nullptr;

void cfxError(char *msg)
{
    if (cfxModule) {
        cfxModule->sendError("%s", msg);
    } else {
        std::cerr << "CFX error: " << msg << std::endl;
    }
}

int wrapCfxInit(vistle::Module *mod, char *resultfilename, int counts[])
{
    cfxMutex.lock();
    cfxModule = mod;
    int nzones = cfxExportInit(resultfilename, counts);
    cfxExportError(cfxError);
    return nzones;
}

void wrapCfxDone()
{
    cfxExportDone();
    cfxMutex.unlock();
}

} // namespace

using namespace vistle;

ReadCFX::ReadCFX(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_ExportDone = true;

    // file browser parameter
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/mnt/raid/home/hpcjwint/data/cfx/rohr/hlrs_002.res", Parameter::ExistingDirectory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/data/eckerle/HLRS_Visualisierung_01122016/Betriebspunkt_250_3000/Configuration3_001.res", Parameter::ExistingDirectory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/data/MundP/3d_Visualisierung_CFX/Transient_003.res", Parameter::ExistingDirectory);
    m_resultfiledir =
        addStringParameter("resultfile", ".res file with absolute path",
                           "/mnt/raid/data/IET/AXIALZYKLON/120929_ML_AXIALZYKLON_P160_OPT_SSG_AB_V2_STATIONAER/"
                           "Steady_grob_V44_P_test_160_5percent_001.res",
                           Parameter::ExistingFilename);
    setParameterFilters(m_resultfiledir, "Result Files (*.res)");

    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/home/jwinterstein/data/cfx/rohr/hlrs_002.res", Parameter::ExistingDirectory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/home/jwinterstein/data/cfx/rohr/hlrs_inst_002.res", Parameter::ExistingDirectory);
    //m_resultfiledir = addStringParameter("resultfile", ".res file with absolute path","/home/jwinterstein/data/cfx/MundP_3d_Visualisierung/3d_Visualisierung_CFX/Transient_003.res", Parameter::ExistingDirectory);

    // timestep parameters
    m_firsttimestep = addIntParameter("firstTimestep", "start reading the first step at this timestep number", 0);
    setParameterMinimum<Integer>(m_firsttimestep, 0);
    m_lasttimestep = addIntParameter("lastTimestep", "stop reading timesteps at this timestep number", 0);
    setParameterMinimum<Integer>(m_lasttimestep, -1);
    m_timeskip = addIntParameter("timeskip", "skip this many timesteps after reading one", 0);
    setParameterMinimum<Integer>(m_timeskip, 0);

    //use rotated data or not
    m_readDataTransformed =
        addIntParameter("transformData",
                        "if true, the data are read transformed with cfxExportZoneMotionAction(cfxMOTION_IGNORE), if "
                        "false a transformation matrix is added",
                        1, Parameter::Boolean);
    m_readGridTransformed =
        addIntParameter("transformGrid",
                        "if true, the grid coordinates are read transformed with "
                        "cfxExportZoneMotionAction(cfxMOTION_IGNORE), if false a transformation matrix is added",
                        1, Parameter::Boolean);

    //zone selection
    m_zoneSelection = addStringParameter("zones", "select zone numbers e.g. 1,4,6-10", "all", Parameter::Restraint);

    // mesh ports
    m_gridOut = createOutputPort("grid_out1", "volume grid");
    m_polyOut = createOutputPort("poly_out1", "boundary geometry");

    // data ports and data choice parameters
    for (int i = 0; i < NumPorts; ++i) {
        { // data ports
            std::stringstream s;
            s << "data_out" << i;
            m_volumeDataOut.push_back(createOutputPort(s.str(), "volume data"));
        }
        { // data choice parameters
            std::stringstream s;
            s << "data" << i;
            auto p = addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back("(NONE)");
            setParameterChoices(p, choices);
            m_fieldOut.push_back(p);
        }
    }
    //m_readBoundary = addIntParameter("read_boundary", "load the boundary?", 0, Parameter::Boolean);
    m_2dAreaSelection =
        addStringParameter("2d_area", "select boundary or region numbers e.g. 1,4,6-10", "0", Parameter::Restraint);

    // 2d data ports and 2d data choice parameters
    for (int i = 0; i < Num2dPorts; ++i) {
        { // 2d data ports
            std::stringstream s;
            s << "data_2d_out" << i;
            m_2dDataOut.push_back(createOutputPort(s.str(), "boundary data"));
        }
        { // 2d data choice parameters
            std::stringstream s;
            s << "data2d" << i;
            auto p = addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back("(NONE)");
            setParameterChoices(p, choices);
            m_2dOut.push_back(p);
        }
    }

    //particle selection
    m_particleSelection =
        addStringParameter("particle_type", "select particle type e.g. 1,4,6-10", "0", Parameter::Restraint);
    m_particleTime = createOutputPort("particle_time", "particle time");

    // particle data ports and particle choice parameters
    for (int i = 0; i < NumParticlePorts; ++i) {
        { // particle data ports
            std::stringstream s;
            s << "data_particle_out" << i;
            m_particleDataOut.push_back(createOutputPort(s.str(), "particle_data"));
        }
        { // particle choice parameters
            std::stringstream s;
            s << "dataParticle" << i;
            auto p = addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
            std::vector<std::string> choices;
            choices.push_back("(NONE)");
            setParameterChoices(p, choices);
            m_particleOut.push_back(p);
        }
    }
    m_currentVolumedata.resize(NumPorts);
    m_current2dData.resize(Num2dPorts);
    m_currentParticleData.resize(NumParticlePorts + 1);
}

ReadCFX::~ReadCFX()
{
    if (!m_ExportDone)
        wrapCfxDone();
    m_ExportDone = true;
}

IdWithZoneFlag::IdWithZoneFlag(): ID(0), zoneFlag(0)
{}

IdWithZoneFlag::IdWithZoneFlag(index_t r, index_t z): ID(r), zoneFlag(z)
{}

Variable::Variable(std::string Name, int Dimension, int onlyMeaningful, int ID, int zone)
: varName(Name), varDimension(Dimension), onlyMeaningfulOnBoundary(onlyMeaningful)
{
    vectorIdwithZone.push_back(IdWithZoneFlag(ID, zone));
}

Particle::Particle(std::string Type, std::string Name, int Dimension, int ID, int zone)
: particleType(Type), varName(Name), varDimension(Dimension), idWithZone(IdWithZoneFlag(ID, zone))
{}

Boundary::Boundary(std::string Name, int ID, int zone): boundName(Name), idWithZone(IdWithZoneFlag(ID, zone))
{}

Region::Region(std::string Name, int ID, int zone): regionName(Name), idWithZone(IdWithZoneFlag(ID, zone))
{}

CaseInfo::CaseInfo(): m_valid(false)
{}

bool CaseInfo::checkFile(const char *filename)
{
    //checks if the file is a valid CFX result file. Every CFX result file starts with "*INFO"

    const int MIN_FILE_SIZE = 1024; // minimal size for .res files [in Byte]

    const int MACIC_LEN = 5; // "magic" at the start
    const char *magic = "*INFO";
    char magicBuf[MACIC_LEN];

    boost::uintmax_t fileSize;
    boost::system::error_code ec;

    FILE *fi = fopen(filename, "rb");

    if (!fi) {
        std::cout << filename << strerror(errno) << std::endl;
        return false;
    } else {
        fileSize = fs::file_size(filename, ec);
        if (ec)
            std::cout << "error code: " << ec << std::endl;
    }

    if (fileSize < MIN_FILE_SIZE) {
        std::cout << filename << "too small to be a real result file" << std::endl;
        std::cout << fileSize << "filesize" << std::endl;
        fclose(fi);
        return false;
    }

    size_t iret = fread(magicBuf, 1, MACIC_LEN, fi);
    fclose(fi);
    if (iret != MACIC_LEN) {
        std::cout << "checkFile :: error reading MACIC_LEN " << std::endl;
        return false;
    }

    if (strncmp(magicBuf, magic, MACIC_LEN) != 0) {
        std::cout << filename << "does not start with '*INFO'" << std::endl;
        return false;
    }

    return true;
}

bool CaseInfo::checkWhichVariablesAreInTransientFile(index_t ntimesteps)
{
    //checks if a variable is in the transient files available or not. Not every variable out of the .res file must be in the .trn files as well.
    //mostly because of memory reasons, the .trn files have less variables

    m_variableInTransientFile.clear();
    if (ntimesteps > 0) {
        cfxExportTimestepSet(cfxExportTimestepNumGet(1));
        index_t nvars = cfxExportVariableCount(ReadCFX::usr_level);

        for (index_t varnum = 1; varnum <= nvars;
             varnum++) { //starts from 1 because cfxExportVariableName(varnum,ReadCFX::alias) only returns values from 1 and higher
            m_variableInTransientFile.push_back(cfxExportVariableName(varnum, ReadCFX::alias));
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

void CaseInfo::parseResultfile()
{
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

    for (index_t i = 1; i <= nzones; ++i) {
        cfxExportZoneSet(i, NULL);

        //read all Variable into m_allParam vector
        nvars = cfxExportVariableCount(ReadCFX::usr_level);
        //std::cerr << "nvars in zone(" << i << ") = " << nvars << std::endl;

        for (index_t varnum = 1; varnum <= nvars;
             varnum++) { //starts from 1 because cfxExportVariableName(varnum,ReadCFX::alias) only returns values from 1 and higher
            const char *VariableName = cfxExportVariableName(varnum, ReadCFX::alias);
            auto it = find_if(m_allParam.begin(), m_allParam.end(),
                              [&VariableName](const Variable &obj) { return obj.varName == VariableName; });
            if (it == m_allParam.end()) {
                if (!cfxExportVariableSize(varnum, &dimension, &length, &corrected_boundary_node)) {
                    std::cerr << "variable if out of range in (parseResultfile -> cfxExportVariableSize)" << std::endl;
                }
                m_allParam.push_back(Variable(VariableName, dimension, length, varnum, i));
                m_numberOfVariables++;
            } else {
                auto index = std::distance(m_allParam.begin(), it);
                m_allParam[index].vectorIdwithZone.push_back(IdWithZoneFlag(varnum, i));
            }
            cfxExportVariableFree(varnum);
        }

        //read all particles into m_allParticle vector
        index_t nparticleTypes = cfxExportGetNumberOfParticleTypes();
        for (index_t particleType = 1; particleType <= nparticleTypes; ++particleType) {
            std::string particleTypeName = cfxExportGetParticleName(particleType);
            index_t nparticleVarCount = cfxExportGetParticleTypeVarCount(particleType);
            for (index_t particleVarNum = 1; particleVarNum <= nparticleVarCount; ++particleVarNum) {
                const char *VariableName =
                    cfxExportGetParticleTypeVarName(particleType, particleVarNum, ReadCFX::alias);
                m_allParticle.push_back(Particle(particleTypeName, VariableName,
                                                 cfxExportGetParticleTypeVarDimension(particleType, particleVarNum),
                                                 particleVarNum, i));
                m_numberOfParticles++;
            }
        }

        //read all Boundaries into m_allBoundaries vector
        nbounds = cfxExportBoundaryCount();
        for (index_t boundnum = 1; boundnum <= nbounds; boundnum++) {
            m_allBoundaries.push_back(Boundary(cfxExportBoundaryName(boundnum), boundnum, i));
            m_numberOfBoundaries++;
        }

        //read all Boundaries into m_allBoundaries vector
        nregions = cfxExportRegionCount();
        for (index_t regnum = 1; regnum <= nregions; regnum++) {
            m_allRegions.push_back(Region(cfxExportRegionName(regnum), regnum, i));
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

void CaseInfo::getFieldList()
{
    //field list for 3D data gets only variables that have meaningful values in the field
    //the field list for the 2D area (boundary) has all available variables

    m_boundary_param.clear();
    m_boundary_param.push_back("(NONE)");
    m_field_param.clear();
    m_field_param.push_back("(NONE)");
    m_particle_types.clear();
    m_particle_types.push_back("(NONE)");

    for (index_t varnum = 0; varnum < m_numberOfVariables; ++varnum) {
        if (m_allParam[varnum].onlyMeaningfulOnBoundary != 1) {
            m_field_param.push_back(m_allParam[varnum].varName);
        }
        m_boundary_param.push_back(m_allParam[varnum].varName);
    }
    for (index_t particleVarNum = 0; particleVarNum < m_numberOfParticles; ++particleVarNum) {
        if (!strcmp((m_allParticle[particleVarNum].particleType).c_str(), (m_allParticle[0].particleType).c_str())) {
            std::string varName = m_allParticle[particleVarNum].varName;
            varName
                .pop_back(); //because each variable name has the typenumber at the end; in the list a general specifier is listed
            m_particle_types.push_back(varName);
        }
    }
    return;
}

std::vector<Variable> CaseInfo::getCopyOfAllParam()
{
    return m_allParam;
}

std::vector<Boundary> CaseInfo::getCopyOfAllBoundaries()
{
    return m_allBoundaries;
}

std::vector<Region> CaseInfo::getCopyOfAllRegions()
{
    return m_allRegions;
}

std::vector<Particle> CaseInfo::getCopyOfAllParticles()
{
    return m_allParticle;
}

std::vector<std::string> CaseInfo::getCopyOfTrnVars()
{
    return m_variableInTransientFile;
}

index_t CaseInfo::getNumberOfBoundaries()
{
    return m_numberOfBoundaries;
}

index_t CaseInfo::getNumberOfRegions()
{
    return m_numberOfRegions;
}

int ReadCFX::rankForVolumeAndTimestep(int setMetaTimestep, int volume, int numVolumes) const
{
    //returns a rank between 0 and size(). ranks are continually distributed to processors over timestep and volumes

    int processor;
    processor = setMetaTimestep * numVolumes + volume;
    //    processor = volume;

    return processor % size();
}

int ReadCFX::rankFor2dAreaAndTimestep(int setMetaTimestep, int area2d, int num2dAreas) const
{
    //returns a rank between 0 and size(). ranks are continually distributed to processors over timestep and volumes

    int processor;
    processor = setMetaTimestep * num2dAreas + area2d;
    //    processor = area2d;

    return processor % size();
}

int ReadCFX::trackStartandEndForRank(int rank, int *firstTrackForRank, int *lastTrackForRank, int numberOfTracks)
{
    //function evenly districutes the tracks of a particle type to the ranks and returns the number of blocks

    int tracksPerRank = std::round(numberOfTracks / size());
    if (tracksPerRank == 0) {
        if (rank == (size() - 1)) {
            *firstTrackForRank = 1;
            *lastTrackForRank = numberOfTracks;
        } else {
            *firstTrackForRank = 0;
            *lastTrackForRank = 0;
        }
        return 1;
    } else {
        *firstTrackForRank = rank * tracksPerRank + 1;
        if (rank == (size() - 1)) {
            *lastTrackForRank = numberOfTracks;
        } else {
            *lastTrackForRank = (rank + 1) * tracksPerRank;
        }
    }

    return size();
}

bool ReadCFX::initializeResultfile()
{
    //call cfxExportInit again in order to access data out of resfile again

    assert(m_ExportDone);

    char *resultfileName = strdup(m_resultfiledir->getValue().c_str());
    m_nzones = wrapCfxInit(this, resultfileName, counts);
    m_ExportDone = false;
    free(resultfileName);
    m_nnodes = counts[cfxCNT_NODE];
    m_nregions = counts[cfxCNT_REGION];
    m_nvolumes = counts[cfxCNT_VOLUME];
    //    m_nelems = counts[cfxCNT_ELEMENT];
    //    m_nvars = counts[cfxCNT_VARIABLE];

    if (m_nzones < 0) {
        wrapCfxDone();
        m_ExportDone = true;
        sendError("cfxExportInit could not open %s", resultfileName);
        return false;
    }

    return true;
}

bool ReadCFX::changeParameter(const Parameter *p)
{
    //set some initialization variables if another resfile is loaded
    if (p == m_resultfiledir) {
        std::string c = m_resultfiledir->getValue();
        const char *resultfiledir;
        resultfiledir = c.c_str();
        m_case.m_valid = m_case.checkFile(resultfiledir);
        if (!m_case.m_valid) {
            sendError("%s is not a valid CFX .res file", resultfiledir);
            return false;
        }

        if (m_nzones > 0) {
            if (!m_ExportDone)
                wrapCfxDone();
            m_ExportDone = true;
        }

        if (initializeResultfile()) {
            if (cfxExportTimestepNumGet(1) < 0) {
                sendInfo("no timesteps");
            }
            m_ntimesteps = cfxExportTimestepCount();
            if (m_ntimesteps > 0) {
                setParameterMaximum<Integer>(m_lasttimestep, m_ntimesteps - 1);
                setParameter<Integer>(m_lasttimestep, m_ntimesteps - 1);
                setParameterMaximum<Integer>(m_firsttimestep, m_ntimesteps - 1);
                setParameterMaximum<Integer>(m_timeskip, m_ntimesteps - 1);
            } else {
                setParameterMaximum<Integer>(m_lasttimestep, 0);
                setParameter<Integer>(m_lasttimestep, 0);
                setParameterMaximum<Integer>(m_firsttimestep, 0);
                setParameter<Integer>(m_firsttimestep, 0);
                setParameterMaximum<Integer>(m_timeskip, 0);
            }

            //fill choice parameter
            m_case.parseResultfile();
            m_case.checkWhichVariablesAreInTransientFile(m_ntimesteps);
            wrapCfxDone();
            m_ExportDone = true;
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
            if (rank() == 0) {
                //print out zone names
                sendInfo("Found %ld zones", (long)m_nzones);
                for (index_t i = 1; i <= m_nzones; i++) {
                    cfxExportZoneSet(i, NULL);
                    sendInfo("zone no. %lu: %s", (unsigned long)i, cfxExportZoneName(i));
                    cfxExportZoneFree();
                }
                //print out 2D area names (boundaries and regions)
                sendInfo("Found %ld boundaries and %ld regions", (long)m_case.getNumberOfBoundaries(),
                         (long)m_case.getNumberOfRegions());
                std::vector<Boundary> allBoundaries = m_case.getCopyOfAllBoundaries();
                for (index_t i = 1; i <= m_case.getNumberOfBoundaries(); ++i) {
                    sendInfo("boundary no. %lu: %s", (unsigned long)i, (allBoundaries[i - 1].boundName).c_str());
                }
                std::vector<Region> allRegions = m_case.getCopyOfAllRegions();
                for (index_t i = (m_case.getNumberOfBoundaries() + 1);
                     i <= (m_case.getNumberOfBoundaries() + m_case.getNumberOfRegions()); ++i) {
                    sendInfo("region no. %lu: %s", (unsigned long)i,
                             (allRegions[i - m_case.getNumberOfBoundaries() - 1].regionName).c_str());
                }
                //print variable names in .trn file
                std::vector<std::string> trnVars = m_case.getCopyOfTrnVars();
                sendInfo("Found %lu variables in transient files", (unsigned long)trnVars.size());
                int j = 1;
                for (std::vector<std::string>::iterator it = trnVars.begin(); it != trnVars.end(); ++it) {
                    sendInfo("%d. %s", j, (*it).c_str());
                    ++j;
                }
                //print particle type names
                index_t nParticleTypes = cfxExportGetNumberOfParticleTypes();
                for (index_t i = 1; i <= nParticleTypes; ++i) {
                    sendInfo("particle type no. %lu: %s", (unsigned long)i, cfxExportGetParticleName(i));
                }
            }
            cfxExportZoneFree();
#ifdef CFX_DEBUG
            sendInfo("The initialisation was successfully done");
#endif
        }
    }
    if (p == m_readDataTransformed) {
        ignoreZoneMotionForData = !m_readDataTransformed->getValue();
    }
    if (p == m_readGridTransformed) {
        ignoreZoneMotionForGrid = !m_readGridTransformed->getValue();
    }

    return Module::changeParameter(p);
}

UnstructuredGrid::ptr ReadCFX::loadGrid(int area3d)
{
    //load an unstructured grid with connectivity, element and coordinate list. Each area3d gets an own unstructured grid.
    index_t nelmsIn3dArea, nconnectivities, nnodesIn3dArea;
    if (cfxExportZoneSet(m_3dAreasSelected[area3d].zoneFlag, counts) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    cfxExportZoneMotionAction(m_3dAreasSelected[area3d].zoneFlag, ignoreZoneMotionForGrid);
#ifdef PARALLEL_ZONES
    nnodesIn3dArea = cfxExportNodeCount();
    nelmsIn3dArea = cfxExportElementCount();
#else
    nnodesIn3dArea = cfxExportVolumeSize(m_3dAreasSelected[area3d].ID, cfxVOL_NODES);
    nelmsIn3dArea = cfxExportVolumeSize(m_3dAreasSelected[area3d].ID, cfxVOL_ELEMS);
#endif
    nconnectivities = 4 * counts[cfxCNT_TET] + 5 * counts[cfxCNT_PYR] + 6 * counts[cfxCNT_WDG] + 8 * counts[cfxCNT_HEX];

    UnstructuredGrid::ptr grid(new UnstructuredGrid(
        nelmsIn3dArea, nconnectivities,
        nnodesIn3dArea)); //initialized with number of elements, number of connectivities, number of coordinates

    //load coords into unstructured grid
    auto ptrOnXcoords = grid->x().data();
    auto ptrOnYcoords = grid->y().data();
    auto ptrOnZcoords = grid->z().data();

#ifdef PARALLEL_ZONES
    cfxNode *nodes = cfxExportNodeList();
    for (index_t i = 0; i < nnodesIn3dArea; ++i) {
        ptrOnXcoords[i] = nodes[i].x;
        ptrOnYcoords[i] = nodes[i].y;
        ptrOnZcoords[i] = nodes[i].z;
    }
#else
    //boost::shared_ptr<std::double_t> x_coord(new double), y_coord(new double), z_coord(new double);
    int *nodeListOfVolume =
        cfxExportVolumeList(m_3dAreasSelected[area3d].ID, cfxVOL_NODES); //query the nodes that define the volume
    index_t nnodesInZone = cfxExportNodeCount();
    std::vector<std::int32_t> nodeListOfVolumeVec;
    nodeListOfVolumeVec.resize(nnodesInZone + 1);
    for (index_t i = 0; i < nnodesIn3dArea; ++i) {
        double xc, yc, zc;
        cfxExportNodeGet(nodeListOfVolume[i], &xc, &yc, &zc); //get access to coordinates: [IN] nodeid [OUT] x,y,z
        ptrOnXcoords[i] = xc;
        ptrOnYcoords[i] = yc;
        ptrOnZcoords[i] = zc;
        nodeListOfVolumeVec[nodeListOfVolume[i]] = i;
    }
#endif
    cfxExportNodeFree();
    //Verification
    //    std::cerr << "m_nnodes = " << m_nnodes << std::endl;
    //    std::cerr << "nnodesIn3dArea = " << nnodesIn3dArea << std::endl;
    //    std::cerr << "nelmsIn3dArea = " << nelmsIn3dArea << "; tets = " << counts[cfxCNT_TET] << ", " << "pyramid = " << counts[cfxCNT_PYR] << ", "<< "prism = " << counts[cfxCNT_WDG] << ", "<< "hex = " << counts[cfxCNT_HEX] << std::endl;
    //    std::cerr << "grid->getNumCoords()" << grid->getNumCoords() << std::endl;
    //    if(m_3dAreasSelected[area3d].zoneFlag==3) {
    //        for(int i=50;i<150;++i) {
    //                std::cerr <<  i << " : " << grid->x().at(i) << ", " << grid->y().at(i) << ", " << grid->z().at(i) << std::endl;
    //            }
    //    }
    //    std::cerr << "x,y,z (10)" << grid->x().at(10) << ", " << grid->y().at(10) << ", " << grid->z().at(10) << std::endl;

    //load element types, element list and connectivity list into unstructured grid
    int elemListCounter = 0;
    auto ptrOnTl = grid->tl().data();
    auto ptrOnEl = grid->el().data();
    auto ptrOnCl = grid->cl().data();

#ifdef PARALLEL_ZONES
    cfxElement *elems = cfxExportElementList();
    for (index_t i = 0; i < nelmsIn3dArea; ++i) {
        switch (elems[i].type) {
        case 4: {
            ptrOnTl[i] = (UnstructuredGrid::TETRAHEDRON);
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter = 0; nodesOfElm_counter < 4; ++nodesOfElm_counter) {
                ptrOnCl[elemListCounter + nodesOfElm_counter] =
                    elems[i].nodeid[nodesOfElm_counter] -
                    1; //cfx starts nodeid with 1; index of coordinate list starts with 0. index coordinate list = nodeid
            }
            elemListCounter += 4;
            break;
        }
        case 5: {
            ptrOnTl[i] = (UnstructuredGrid::PYRAMID);
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter = 0; nodesOfElm_counter < 5; ++nodesOfElm_counter) {
                ptrOnCl[elemListCounter + nodesOfElm_counter] = elems[i].nodeid[nodesOfElm_counter] - 1;
            }
            elemListCounter += 5;
            break;
        }
        case 6: {
            ptrOnTl[i] = (UnstructuredGrid::PRISM);
            ptrOnEl[i] = elemListCounter;

            // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
            ptrOnCl[elemListCounter + 0] = elems[i].nodeid[3] - 1;
            ptrOnCl[elemListCounter + 1] = elems[i].nodeid[5] - 1;
            ptrOnCl[elemListCounter + 2] = elems[i].nodeid[4] - 1;
            ptrOnCl[elemListCounter + 3] = elems[i].nodeid[0] - 1;
            ptrOnCl[elemListCounter + 4] = elems[i].nodeid[2] - 1;
            ptrOnCl[elemListCounter + 5] = elems[i].nodeid[1] - 1;
            elemListCounter += 6;
            break;
        }
        case 8: {
            ptrOnTl[i] = (UnstructuredGrid::HEXAHEDRON);
            ptrOnEl[i] = elemListCounter;

            // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
            ptrOnCl[elemListCounter + 0] = elems[i].nodeid[4] - 1;
            ptrOnCl[elemListCounter + 1] = elems[i].nodeid[6] - 1;
            ptrOnCl[elemListCounter + 2] = elems[i].nodeid[7] - 1;
            ptrOnCl[elemListCounter + 3] = elems[i].nodeid[5] - 1;
            ptrOnCl[elemListCounter + 4] = elems[i].nodeid[0] - 1;
            ptrOnCl[elemListCounter + 5] = elems[i].nodeid[2] - 1;
            ptrOnCl[elemListCounter + 6] = elems[i].nodeid[3] - 1;
            ptrOnCl[elemListCounter + 7] = elems[i].nodeid[1] - 1;

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
    int *elmListOfVolume =
        cfxExportVolumeList(m_3dAreasSelected[area3d].ID, cfxVOL_ELEMS); //query the elements that define the volume
    for (index_t i = 0; i < nelmsIn3dArea; ++i) {
        cfxExportElementGet(elmListOfVolume[i], elemtype, nodesOfElm);
        switch (*elemtype) {
        case 4: {
            ptrOnTl[i] = (UnstructuredGrid::TETRAHEDRON);
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter = 0; nodesOfElm_counter < 4; ++nodesOfElm_counter) {
                ptrOnCl[elemListCounter + nodesOfElm_counter] = nodeListOfVolumeVec[nodesOfElm[nodesOfElm_counter]];
            }
            elemListCounter += 4;
            break;
        }
        case 5: {
            ptrOnTl[i] = (UnstructuredGrid::PYRAMID);
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter = 0; nodesOfElm_counter < 5; ++nodesOfElm_counter) {
                ptrOnCl[elemListCounter + nodesOfElm_counter] = nodeListOfVolumeVec[nodesOfElm[nodesOfElm_counter]];
            }
            elemListCounter += 5;
            break;
        }
        case 6: {
            ptrOnTl[i] = (UnstructuredGrid::PRISM);
            ptrOnEl[i] = elemListCounter;

            // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
            ptrOnCl[elemListCounter + 0] = nodeListOfVolumeVec[nodesOfElm[3]];
            ptrOnCl[elemListCounter + 1] = nodeListOfVolumeVec[nodesOfElm[5]];
            ptrOnCl[elemListCounter + 2] = nodeListOfVolumeVec[nodesOfElm[4]];
            ptrOnCl[elemListCounter + 3] = nodeListOfVolumeVec[nodesOfElm[0]];
            ptrOnCl[elemListCounter + 4] = nodeListOfVolumeVec[nodesOfElm[2]];
            ptrOnCl[elemListCounter + 5] = nodeListOfVolumeVec[nodesOfElm[1]];
            elemListCounter += 6;
            break;
        }
        case 8: {
            ptrOnTl[i] = (UnstructuredGrid::HEXAHEDRON);
            ptrOnEl[i] = elemListCounter;

            // indizee through comparison of Covise->Programmer's guide->COVISE Data Objects->Unstructured Grid Types with CFX Reference Guide p. 54
            ptrOnCl[elemListCounter + 0] = nodeListOfVolumeVec[nodesOfElm[4]];
            ptrOnCl[elemListCounter + 1] = nodeListOfVolumeVec[nodesOfElm[6]];
            ptrOnCl[elemListCounter + 2] = nodeListOfVolumeVec[nodesOfElm[7]];
            ptrOnCl[elemListCounter + 3] = nodeListOfVolumeVec[nodesOfElm[5]];
            ptrOnCl[elemListCounter + 4] = nodeListOfVolumeVec[nodesOfElm[0]];
            ptrOnCl[elemListCounter + 5] = nodeListOfVolumeVec[nodesOfElm[2]];
            ptrOnCl[elemListCounter + 6] = nodeListOfVolumeVec[nodesOfElm[3]];
            ptrOnCl[elemListCounter + 7] = nodeListOfVolumeVec[nodesOfElm[1]];

            elemListCounter += 8;
            break;
        }
        default: {
            std::cerr << "Elementtype(" << *elemtype << "not yet implemented." << std::endl;
        }
        }
    }
    cfxExportVolumeFree(m_3dAreasSelected[area3d].ID);
    grid->cl().resize(
        elemListCounter); //correct initialization; initialized with connectivities in zone but connectivities in 3dArea are less equal (<=) than connectivities in zone
#endif

    //element after last element
    ptrOnEl[nelmsIn3dArea] = elemListCounter;

    //Verification:
    //    std::cerr << "tets = " << counts[cfxCNT_TET] << "; pyramids = " << counts[cfxCNT_PYR] << "; prism = " << counts[cfxCNT_WDG] << "; hexaeder = " << counts[cfxCNT_HEX] << std::endl;
    //    std::cerr <<"no. elems total = " << m_nelems << std::endl;
    //    std::cerr <<"grid->getNumElements" << grid->getNumElements() << std::endl;
    //    for(index_t i = nelmsIn3dArea-5;i<nelmsIn3dArea;++i) {
    //        std::cerr << "tl(" << i << ") = " << grid->tl().at(i) << std::endl;
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

Polygons::ptr ReadCFX::loadPolygon(int area2d)
{
    //load polygon with connectivity, element and coordinate list. Each area2d gets an own polygon
    if (cfxExportZoneSet(m_2dAreasSelected[area2d].idWithZone.zoneFlag, counts) <
        0) { //counts is a vector for statistics of the zone
        std::cerr << "invalid zone number" << std::endl;
    }
    cfxExportZoneMotionAction(m_2dAreasSelected[area2d].idWithZone.zoneFlag, ignoreZoneMotionForGrid);
    int *nodeListOf2dArea = nullptr;
    index_t nNodesIn2dArea = 0, nFacesIn2dArea = 0;

    if (m_2dAreasSelected[area2d].boundary) {
        nNodesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[area2d].idWithZone.ID, cfxREG_NODES);
        nodeListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[area2d].idWithZone.ID,
                                                 cfxREG_NODES); //query the nodes that define the 2dArea
        nFacesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[area2d].idWithZone.ID, cfxREG_FACES);
    } else {
        nNodesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[area2d].idWithZone.ID, cfxREG_NODES);
        nodeListOf2dArea = cfxExportRegionList(m_2dAreasSelected[area2d].idWithZone.ID,
                                               cfxREG_NODES); //query the nodes that define the 2dArea
        nFacesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[area2d].idWithZone.ID, cfxREG_FACES);
    }

    index_t nConnectIn2dArea =
        4 *
        nFacesIn2dArea; //maximum of conncectivities. If there are 3 vertices faces, it is corrected with resize at the end of the function

    Polygons::ptr polygon(new Polygons(nFacesIn2dArea, nConnectIn2dArea,
                                       nNodesIn2dArea)); //initialize Polygon with numFaces, numCorners, numVertices

    //load coords into polygon
    auto ptrOnXcoords = polygon->x().data();
    auto ptrOnYcoords = polygon->y().data();
    auto ptrOnZcoords = polygon->z().data();

    index_t nNodesInZone = cfxExportNodeCount();
    std::vector<std::int32_t> nodeListOf2dAreaVec;
    nodeListOf2dAreaVec.resize(nNodesInZone + 1);

    double x_coord, y_coord, z_coord;
    for (index_t i = 0; i < nNodesIn2dArea; ++i) {
        cfxExportNodeGet(nodeListOf2dArea[i], &x_coord, &y_coord,
                         &z_coord); //get access to coordinates: [IN] nodeid [OUT] x,y,z
        ptrOnXcoords[i] = x_coord;
        ptrOnYcoords[i] = y_coord;
        ptrOnZcoords[i] = z_coord;
        nodeListOf2dAreaVec[nodeListOf2dArea[i]] = i;
    }

    //Verification
    //        std::cerr << "nNodesIn2dArea = " << nNodesIn2dArea << std::endl;
    //        std::cerr << "polygon->getNumCoords()" << polygon->getNumCoords() << std::endl;
    //        for(int i=0;i<100;++i) {
    //            std::cerr << "x,y,z (" << i << ") = " << polygon->x().at(i) << ", " << polygon->y().at(i) << ", " << polygon->z().at(i) << std::endl;
    //        }

    cfxExportNodeFree();
    free2dArea(m_2dAreasSelected[area2d].boundary, area2d);

    //attention: cfxExportBoundary/RegionList(cfxREG_FACES) has to be called AFTER cfxExportBoundary/RegionList(cfxREG_NODES) and free2dArea
    //if not, cfxExportBoundary/RegionList(cfxREG_NODES) gives unreasonable values for node id's
    int *faceListOf2dArea = nullptr;
    if (m_2dAreasSelected[area2d].boundary) {
        faceListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[area2d].idWithZone.ID,
                                                 cfxREG_FACES); //query the faces that define the 2dArea
    } else {
        faceListOf2dArea = cfxExportRegionList(m_2dAreasSelected[area2d].idWithZone.ID,
                                               cfxREG_FACES); //query the faces that define the 2dArea
    }

    //load face types, element list and connectivity list into polygon
    int elemListCounter = 0;
    //boost::shared_ptr<std::int32_t> nodesOfFace(new int[4]);
    std::int32_t nodesOfFace[4];
    auto ptrOnEl = polygon->el().data();
    auto ptrOnCl = polygon->cl().data();

    for (index_t i = 0; i < nFacesIn2dArea; ++i) {
        int NumOfVerticesDefiningFace = cfxExportFaceNodes(faceListOf2dArea[i], nodesOfFace);
        assert(NumOfVerticesDefiningFace <= 4); //see CFX Reference Guide p.57
        switch (NumOfVerticesDefiningFace) {
        case 3: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter = 0; nodesOfElm_counter < 3; ++nodesOfElm_counter) {
                ptrOnCl[elemListCounter + nodesOfElm_counter] = nodeListOf2dAreaVec[nodesOfFace[nodesOfElm_counter]];
            }
            elemListCounter += 3;
            break;
        }
        case 4: {
            ptrOnEl[i] = elemListCounter;
            for (int nodesOfElm_counter = 0; nodesOfElm_counter < 4; ++nodesOfElm_counter) {
                ptrOnCl[elemListCounter + nodesOfElm_counter] = nodeListOf2dAreaVec[nodesOfFace[nodesOfElm_counter]];
            }
            elemListCounter += 4;
            break;
        }
        default: {
            std::cerr << "number of vertices defining the face(" << NumOfVerticesDefiningFace << ") not implemented."
                      << std::endl;
        }
        }
    }

    //element after last element in element list and connectivity list
    polygon->cl().resize(elemListCounter);
    ptrOnEl[nFacesIn2dArea] = elemListCounter;

    //Verification
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

    free2dArea(m_2dAreasSelected[area2d].boundary, area2d);

    return polygon;
}

vistle::Lines::ptr ReadCFX::loadParticleTrackCoords(int particleTypeNumber, const index_t numVertices,
                                                    int firstTrackForRank, int lastTrackForRank)
{
    //reads all tracks one after another in a lines object with x,y,z coordinates, connectivity list and element list

    Lines::ptr lines(new Lines(
        (lastTrackForRank - firstTrackForRank) + 1, numVertices,
        numVertices)); //element list and corner list are initialized with number of vertices, because only "dot elements" are used in the line
    auto ptrOnXCoords = lines->x().data();
    auto ptrOnYCoords = lines->y().data();
    auto ptrOnZCoords = lines->z().data();
    auto ptrOnEl = lines->el().data();
    auto ptrOnCl = lines->cl().data();

    int pointsOnTrackCounter = 0, elemListCounter = 0;
    for (int i = firstTrackForRank; i <= lastTrackForRank; ++i) {
        int NumOfPointsOnTrack;
        float *ParticleTrackCoords =
            cfxExportGetParticleTrackCoordinatesByTrack(particleTypeNumber, i, &NumOfPointsOnTrack);
        ptrOnEl[elemListCounter] = pointsOnTrackCounter;
        for (int k = 0; k < NumOfPointsOnTrack; ++k) {
            ptrOnXCoords[pointsOnTrackCounter + k] = ParticleTrackCoords[k * 3];
            ptrOnYCoords[pointsOnTrackCounter + k] = ParticleTrackCoords[k * 3 + 1];
            ptrOnZCoords[pointsOnTrackCounter + k] = ParticleTrackCoords[k * 3 + 2];
            ptrOnCl[pointsOnTrackCounter + k] = pointsOnTrackCounter + k;
        }
        pointsOnTrackCounter += NumOfPointsOnTrack;
        elemListCounter++;
    }

    //element entry after last element
    ptrOnEl[elemListCounter] = pointsOnTrackCounter;

    return lines;
}

DataBase::ptr ReadCFX::loadField(int area3d, Variable var)
{
    //load scalar data in Vec and vector data in Vec3 for the area3d
    for (index_t i = 0; i < var.vectorIdwithZone.size(); ++i) {
        if (var.vectorIdwithZone[i].zoneFlag == m_3dAreasSelected[area3d].zoneFlag) {
            if (cfxExportZoneSet(m_3dAreasSelected[area3d].zoneFlag, NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
            }
            cfxExportZoneMotionAction(m_3dAreasSelected[area3d].zoneFlag, ignoreZoneMotionForData);

#ifdef PARALLEL_ZONES
            index_t nnodesInZone = cfxExportNodeCount();
            //read field parameters
            index_t varnum = var.vectorIdwithZone[i].ID;
            float *variableList = cfxExportVariableList(varnum, correct);
            if (var.varDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nnodesInZone));
                scalar_t *ptrOnScalarData = s->x().data();
                for (index_t j = 0; j < nnodesInZone; ++j) {
                    ptrOnScalarData[j] = variableList[j];
                    //                    if(j<10) {
                    //                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
                    //                    }
                }
                cfxExportVariableFree(varnum);
                return s;
            } else if (var.varDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nnodesInZone));
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();

                for (index_t j = 0; j < nnodesInZone; ++j) {
                    ptrOnVectorXData[j] = variableList[3 * j];
                    ptrOnVectorYData[j] = variableList[3 * j + 1];
                    ptrOnVectorZData[j] = variableList[3 * j + 2];
                    //                    if(j<100 || j>(nnodesInZone-100)) {
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
            index_t nnodesInVolume = cfxExportVolumeSize(m_3dAreasSelected[area3d].ID, cfxVOL_NODES);
            int *nodeListOfVolume = cfxExportVolumeList(m_3dAreasSelected[area3d].ID,
                                                        cfxVOL_NODES); //query the nodes that define the volume

            //read field parameters
            index_t varnum = var.vectorIdwithZone[i].ID;

            if (var.varDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nnodesInVolume));
                scalar_t *ptrOnScalarData = s->x().data();

                for (index_t j = 0; j < nnodesInVolume; ++j) {
                    float value;
                    cfxExportVariableGet(varnum, correct, nodeListOfVolume[j], &value);
                    ptrOnScalarData[j] = value;
                    //                    if(j<10) {
                    //                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
                    //                    }
                }
                //cfxExportVariableFree(varnum);
                cfxExportVolumeFree(m_3dAreasSelected[area3d].ID);
                return s;
            } else if (var.varDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nnodesInVolume));
                float value[3];
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();

                for (index_t j = 0; j < nnodesInVolume; ++j) {
                    cfxExportVariableGet(varnum, correct, nodeListOfVolume[j], value);
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

bool ReadCFX::free2dArea(bool boundary, int area2d)
{
    //function to call cfxExportFree... depending which kind of 2d area is read.
    //cfx API requires that after a call of cfxBoundaryGet or cfxRegionGet the memory needs to be freed
    //area2d can either be boundaries or regions
    if (boundary) {
        cfxExportBoundaryFree(m_2dAreasSelected[area2d].idWithZone.ID);
    } else {
        cfxExportRegionFree(m_2dAreasSelected[area2d].idWithZone.ID);
    }

    return true;
}

DataBase::ptr ReadCFX::load2dField(int area2d, Variable var)
{
    //load scalar data in Vec and vector data in Vec3 for the area2d
    for (index_t i = 0; i < var.vectorIdwithZone.size(); ++i) {
        if (m_2dAreasSelected[area2d].idWithZone.zoneFlag == var.vectorIdwithZone[i].zoneFlag) {
            if (cfxExportZoneSet(m_2dAreasSelected[area2d].idWithZone.zoneFlag, NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
            }

            cfxExportZoneMotionAction(m_3dAreasSelected[area2d].zoneFlag, ignoreZoneMotionForData);

            index_t nNodesIn2dArea = 0;
            int *nodeListOf2dArea;
            if (m_2dAreasSelected[area2d].boundary) {
                nNodesIn2dArea = cfxExportBoundarySize(m_2dAreasSelected[area2d].idWithZone.ID, cfxREG_NODES);
                nodeListOf2dArea = cfxExportBoundaryList(m_2dAreasSelected[area2d].idWithZone.ID,
                                                         cfxREG_NODES); //query the nodes that define the 2dArea
            } else {
                nNodesIn2dArea = cfxExportRegionSize(m_2dAreasSelected[area2d].idWithZone.ID, cfxREG_NODES);
                nodeListOf2dArea = cfxExportRegionList(m_2dAreasSelected[area2d].idWithZone.ID,
                                                       cfxREG_NODES); //query the nodes that define the 2dArea
            }

            //read field parameters
            index_t varnum = var.vectorIdwithZone[i].ID;
            if (var.varDimension == 1) {
                Vec<Scalar>::ptr s(new Vec<Scalar>(nNodesIn2dArea));
                scalar_t *ptrOnScalarData = s->x().data();
                float value;
                for (index_t j = 0; j < nNodesIn2dArea; ++j) {
                    cfxExportVariableGet(varnum, correct, nodeListOf2dArea[j], &value);
                    ptrOnScalarData[j] = value;
                    //                    if(j<20) {
                    //                        std::cerr << "ptrOnScalarData[" << j << "] = " << ptrOnScalarData[j] << std::endl;
                    //                    }
                }
                free2dArea(m_2dAreasSelected[area2d].boundary, area2d);
                return s;
            } else if (var.varDimension == 3) {
                Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(nNodesIn2dArea));
                float value[3];
                scalar_t *ptrOnVectorXData, *ptrOnVectorYData, *ptrOnVectorZData;
                ptrOnVectorXData = v->x().data();
                ptrOnVectorYData = v->y().data();
                ptrOnVectorZData = v->z().data();
                for (index_t j = 0; j < nNodesIn2dArea; ++j) {
                    cfxExportVariableGet(varnum, correct, nodeListOf2dArea[j], value);
                    ptrOnVectorXData[j] = value[0];
                    ptrOnVectorYData[j] = value[1];
                    ptrOnVectorZData[j] = value[2];
                    //                    if(j<100) {
                    //                        std::cerr << "ptrOnVectorXData[" << j << "] = " << ptrOnVectorXData[j] << std::endl;
                    //                        std::cerr << "ptrOnVectorYData[" << j << "] = " << ptrOnVectorYData[j] << std::endl;
                    //                        std::cerr << "ptrOnVectorZData[" << j << "] = " << ptrOnVectorZData[j] << std::endl;
                    //                    }
                }
                free2dArea(m_2dAreasSelected[area2d].boundary, area2d);
                return v;
            }
            free2dArea(m_2dAreasSelected[area2d].boundary, area2d);
        }
    }
    return DataBase::ptr();
}

vistle::DataBase::ptr ReadCFX::loadParticleTime(int particleTypeNumber, const index_t NumVertices,
                                                int firstTrackForRank, int lastTrackForRank)
{
    // loads the time values for each point on every track of the particle type

    int pointsOnTrackCounter = 0;
    Vec<Scalar>::ptr s(new Vec<Scalar>(NumVertices));
    scalar_t *ptrOnScalarData = s->x().data();
    for (int j = firstTrackForRank; j <= lastTrackForRank; ++j) {
        int NumOfPointsOnTrack;
        float *ParticleTrackTime = cfxExportGetParticleTrackTimeByTrack(particleTypeNumber, j, &NumOfPointsOnTrack);
        for (int k = 0; k < NumOfPointsOnTrack; ++k) {
            ptrOnScalarData[pointsOnTrackCounter + k] = ParticleTrackTime[k];
        }
        pointsOnTrackCounter += NumOfPointsOnTrack;
    }

    return s;
}


vistle::DataBase::ptr ReadCFX::loadParticleValues(int particleTypeNumber, Particle particle, const index_t NumVertices,
                                                  int firstTrackForRank, int lastTrackForRank)
{
    //loads the values for each point on every track of the given particle type and variable

    int pointsOnTrackCounter = 0;

    if (particle.varDimension == 1) {
        Vec<Scalar>::ptr s(new Vec<Scalar>(NumVertices));
        scalar_t *ptrOnScalarData = s->x().data();
        for (int j = firstTrackForRank; j <= lastTrackForRank; ++j) {
            int NumOfPointsOnTrack = cfxExportGetNumberOfPointsOnParticleTrack(particleTypeNumber, j);
            float *particleVar = cfxExportGetParticleTypeVar(particleTypeNumber, particle.idWithZone.ID, j);
            for (int k = 0; k < NumOfPointsOnTrack; ++k) {
                ptrOnScalarData[pointsOnTrackCounter + k] = particleVar[k];
            }
            pointsOnTrackCounter += NumOfPointsOnTrack;
        }
        return s;
    }
    if (particle.varDimension == 3) {
        Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(NumVertices));
        scalar_t *ptrOnVectorXData = v->x().data();
        scalar_t *ptrOnVectorYData = v->y().data();
        scalar_t *ptrOnVectorZData = v->z().data();
        for (int j = firstTrackForRank; j <= lastTrackForRank; ++j) {
            int NumOfPointsOnTrack = cfxExportGetNumberOfPointsOnParticleTrack(particleTypeNumber, j);
            float *particleVar = cfxExportGetParticleTypeVar(particleTypeNumber, particle.idWithZone.ID, j);
            for (int k = 0; k < NumOfPointsOnTrack; ++k) {
                ptrOnVectorXData[pointsOnTrackCounter + k] = particleVar[3 * k];
                ptrOnVectorYData[pointsOnTrackCounter + k] = particleVar[3 * k + 1];
                ptrOnVectorZData[pointsOnTrackCounter + k] = particleVar[3 * k + 2];
            }
            pointsOnTrackCounter += NumOfPointsOnTrack;
        }
        return v;
    }
    return DataBase::ptr();
}

index_t ReadCFX::collect3dAreas()
{
    //write either selected zones or volumes (in selected zones) in a vector in a consecutively order

    // read zone selection; m_coRestraintZones contains a bool array of selected zones
    //m_coRestraintZones(zone) = 1 zone is selected
    //m_coRestraintZones(zone) = 0 zone isn't selected
    //group = -1 zones are selected
    m_coRestraintZones.clear();
    m_coRestraintZones.add(m_zoneSelection->getValue());
    ssize_t val = m_coRestraintZones.getNumGroups(), group;
    m_coRestraintZones.get(val, group);

    index_t numberOfSelected3dAreas = 0;
    m_3dAreasSelected.clear();
#ifdef PARALLEL_ZONES
    m_3dAreasSelected.resize(m_nzones);
#else
    m_3dAreasSelected.resize(m_nvolumes);
#endif
    for (index_t i = 1; i <= m_nzones; ++i) {
        if (m_coRestraintZones(i)) {
#ifdef PARALLEL_ZONES
            m_3dAreasSelected[numberOfSelected3dAreas] = IdWithZoneFlag(0, i);
            numberOfSelected3dAreas++;
#else
            if (cfxExportZoneSet(i, NULL) < 0) {
                std::cerr << "invalid zone number" << std::endl;
                return numberOfSelected3dAreas;
            }
            int nvolumes = cfxExportVolumeCount();
            for (int j = 1; j <= nvolumes; ++j) {
                m_3dAreasSelected[numberOfSelected3dAreas] = IdWithZoneFlag(j, i);
                numberOfSelected3dAreas++;
            }
#endif
        }
    }

#ifndef PARALLEL_ZONES
    m_3dAreasSelected.resize(numberOfSelected3dAreas);
#endif


    //Verification
    //    for(index_t i=0;i<numberOfSelected3dAreas;++i) {
    //        std::cerr << "m_3dAreasSelected[" << i << "].ID = " << m_3dAreasSelected[i].ID << " m_3dAreasSelected.zoneFlag" << m_3dAreasSelected[i].zoneFlag << std::endl;
    //    }
    cfxExportZoneFree();
    return numberOfSelected3dAreas;
}

index_t ReadCFX::collect2dAreas()
{
    //write either selected boundaries and regions in a vector in a consecutively order

    //m_coRestraint2dAreas contains a bool array of selected 2dAreas
    //m_coRestraint2dAreas(Area2d) = 1 Area2d is selected
    //m_coRestraint2dAreas(Area2d) = 0 Area2d isn't selected
    //group = -1 all Area2d are selected
    m_coRestraint2dAreas.clear();
    m_coRestraint2dAreas.add(m_2dAreaSelection->getValue());
    ssize_t val = m_coRestraint2dAreas.getNumGroups(), group;
    m_coRestraint2dAreas.get(val, group);

    index_t numberOfSelected2dAreas = 0;
    m_2dAreasSelected.clear();
    m_2dAreasSelected.resize(m_case.getNumberOfBoundaries() + m_case.getNumberOfRegions());
    std::vector<Boundary> allBoundaries = m_case.getCopyOfAllBoundaries();
    std::vector<Region> allRegions = m_case.getCopyOfAllRegions();

    for (index_t i = 1; i <= m_case.getNumberOfBoundaries(); ++i) {
        if (m_coRestraint2dAreas(i)) {
            m_2dAreasSelected[numberOfSelected2dAreas].idWithZone =
                IdWithZoneFlag(allBoundaries[i - 1].idWithZone.ID, allBoundaries[i - 1].idWithZone.zoneFlag);
            m_2dAreasSelected[numberOfSelected2dAreas].boundary = true;
            numberOfSelected2dAreas++;
        }
    }
    for (index_t i = m_case.getNumberOfBoundaries() + 1;
         i <= (m_case.getNumberOfBoundaries() + m_case.getNumberOfRegions()); ++i) {
        if (m_coRestraint2dAreas(i)) {
            m_2dAreasSelected[numberOfSelected2dAreas].idWithZone =
                IdWithZoneFlag(allRegions[i - m_case.getNumberOfBoundaries() - 1].idWithZone.ID,
                               allRegions[i - m_case.getNumberOfBoundaries() - 1].idWithZone.zoneFlag);
            m_2dAreasSelected[numberOfSelected2dAreas].boundary = false;
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

index_t ReadCFX::collectParticles()
{
    //write selected particle types in a vector in consecutively order
    //m_coRestraintParticle contains a bool array of selected Particles
    //m_coRestraintParticle(Area2d) = 1 Particle is selected
    //m_coRestraintParticle(Area2d) = 0 Particle isn't selected
    //group = -1 all particles are selected
    m_coRestraintParticle.clear();
    m_coRestraintParticle.add(m_particleSelection->getValue());
    ssize_t val = m_coRestraintParticle.getNumGroups(), group;
    m_coRestraintParticle.get(val, group);

    index_t numberOfSelectedParticles = 0;
    m_particleTypesSelected.clear();
    index_t nParticleTypes = cfxExportGetNumberOfParticleTypes();
    m_particleTypesSelected.resize(nParticleTypes);

    for (index_t i = 1; i <= nParticleTypes; ++i) {
        if (m_coRestraintParticle(i)) {
            m_particleTypesSelected[numberOfSelectedParticles] = i;
            numberOfSelectedParticles++;
        }
    }
    m_particleTypesSelected.resize(numberOfSelectedParticles);

    //Verification
    if (rank() == 0) {
        for (index_t i = 0; i < numberOfSelectedParticles; ++i) {
            std::cerr << "m_particleTypesSelected[" << i << "] = " << m_particleTypesSelected[i] << std::endl;
        }
    }

    return numberOfSelectedParticles;
}


bool ReadCFX::loadFields(UnstructuredGrid::ptr grid, int area3d, int setMetaTimestep, int timestep, int numTimesteps,
                         index_t numSel3dArea, bool readTransientFile)
{
    //calls for each port the loadGrid and loadField function and the setDataObject function to get the object ready to be added to port

    for (int i = 0; i < NumPorts; ++i) {
        std::string field = m_fieldOut[i]->getValue();
        std::vector<Variable> allParam = m_case.getCopyOfAllParam();
        std::vector<std::string> trnVars = m_case.getCopyOfTrnVars();
        auto it =
            find_if(allParam.begin(), allParam.end(), [&field](const Variable &obj) { return obj.varName == field; });
        if (it == allParam.end()) {
            m_currentVolumedata[i] = DataBase::ptr();
        } else {
            auto index = std::distance(allParam.begin(), it);
            DataBase::ptr obj = loadField(area3d, allParam[index]);
            obj->setMapping(DataBase::Vertex);

            if (std::find(trnVars.begin(), trnVars.end(), allParam[index].varName) == trnVars.end()) {
                //variable exists only in resfile --> timestep = -1
                Matrix4 t =
                    getTransformationMatrix(m_3dAreasSelected[area3d].zoneFlag, timestep, ignoreZoneMotionForData);
                setMeta(obj, area3d, setMetaTimestep, -1, numTimesteps, numSel3dArea, readTransientFile, t);
                obj->setGrid(m_gridsInTimestepForResfile[area3d]);
            } else {
                //variable exists in resfile and in transient files --> timestep = last
                Matrix4 t =
                    getTransformationMatrix(m_3dAreasSelected[area3d].zoneFlag, timestep, ignoreZoneMotionForData);
                setMeta(obj, area3d, setMetaTimestep, timestep, numTimesteps, numSel3dArea, readTransientFile, t);
                obj->setGrid(grid);
            }
            obj->addAttribute(attribute::Species, field);
            m_currentVolumedata[i] = obj;
        }
    }
    return true;
}

bool ReadCFX::load2dFields(Polygons::ptr polyg, int area2d, int setMetaTimestep, int timestep, int numTimesteps,
                           index_t numSel2dArea, bool readTransientFile)
{
    //calls for each port the loadPolygon and load2dField function and the set2dDataObject function to get the object ready to be added to port

    for (int i = 0; i < Num2dPorts; ++i) {
        std::string area2dField = m_2dOut[i]->getValue();
        std::vector<Variable> allParam = m_case.getCopyOfAllParam();
        std::vector<std::string> trnVars = m_case.getCopyOfTrnVars();
        auto it = find_if(allParam.begin(), allParam.end(),
                          [&area2dField](const Variable &obj) { return obj.varName == area2dField; });
        if (it == allParam.end()) {
            m_current2dData[i] = DataBase::ptr();
        } else {
            auto index = std::distance(allParam.begin(), it);
            DataBase::ptr obj = load2dField(area2d, allParam[index]);

            if (std::find(trnVars.begin(), trnVars.end(), allParam[index].varName) == trnVars.end()) {
                //variable exists only in resfile --> timestep = -1
                Matrix4 t = getTransformationMatrix(m_2dAreasSelected[area2d].idWithZone.zoneFlag, timestep,
                                                    ignoreZoneMotionForData);
                setMeta(obj, area2d, setMetaTimestep, -1, numTimesteps, numSel2dArea, readTransientFile, t);
                obj->setMapping(DataBase::Vertex);
                obj->setGrid(m_polygonsInTimestepForResfile[area2d]);
            } else {
                //variable exists in resfile and in transient files --> timestep = last
                Matrix4 t = getTransformationMatrix(m_2dAreasSelected[area2d].idWithZone.zoneFlag, timestep,
                                                    ignoreZoneMotionForData);
                setMeta(obj, area2d, setMetaTimestep, timestep, numTimesteps, numSel2dArea, readTransientFile, t);
                obj->setMapping(DataBase::Vertex);
                obj->setGrid(polyg);
            }
            obj->addAttribute(attribute::Species, area2dField);
            m_current2dData[i] = obj;
        }
    }
    return true;
}

bool ReadCFX::loadParticles(int particleTypeNumber)
{
    //calls loadParticleTrackCoords, loads the time of the particles and calls loadParticleValues for each variable, sets the coords to the values and calls addParticlesToPort

    std::vector<Particle> allParticle = m_case.getCopyOfAllParticles();
    if (cfxExportZoneSet(allParticle[0].idWithZone.zoneFlag, NULL) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    if (m_currentParticleData.size() == 0) {
        m_currentParticleData.resize(NumParticlePorts + 1);
    }
    cfxExportZoneMotionAction(allParticle[0].idWithZone.zoneFlag, ignoreZoneMotionForData);
    int numberOfTracks = cfxExportGetNumberOfTracks(particleTypeNumber);
    int firstTrackForRank, lastTrackForRank;
    int numberOfBlocks = trackStartandEndForRank(rank(), &firstTrackForRank, &lastTrackForRank, numberOfTracks);
    if (firstTrackForRank == 0 && lastTrackForRank == 0)
        return true;

    index_t NumVertices = 0;
    for (int i = firstTrackForRank; i <= lastTrackForRank; ++i) {
        NumVertices += cfxExportGetNumberOfPointsOnParticleTrack(particleTypeNumber, i);
    }
    m_coordsOfParticles = loadParticleTrackCoords(particleTypeNumber, NumVertices, firstTrackForRank, lastTrackForRank);
    m_currentParticleData[0] = loadParticleTime(particleTypeNumber, NumVertices, firstTrackForRank, lastTrackForRank);

    m_coordsOfParticles->setNumBlocks(numberOfBlocks);
    m_currentParticleData[0]->setNumBlocks(numberOfBlocks);
    if (numberOfBlocks > 1) {
        m_coordsOfParticles->setBlock(rank());
        m_currentParticleData[0]->setBlock(rank());
    } else {
        m_coordsOfParticles->setBlock(0);
        m_currentParticleData[0]->setBlock(0);
    }
    m_currentParticleData[0]->setGrid(m_coordsOfParticles);

    for (int i = 0; i < NumParticlePorts; ++i) {
        std::string field = m_particleOut[i]->getValue();
        std::string s = std::to_string(particleTypeNumber);
        field.append(s.c_str());
        auto it = find_if(allParticle.begin(), allParticle.end(),
                          [&field](const Particle &obj) { return obj.varName == field; });
        if (it == allParticle.end()) {
            m_currentParticleData[i + 1] = DataBase::ptr();
        } else {
            auto index = std::distance(allParticle.begin(), it);
            m_currentParticleData[i + 1] = loadParticleValues(particleTypeNumber, allParticle[index], NumVertices,
                                                              firstTrackForRank, lastTrackForRank);
        }
        if (m_currentParticleData[i + 1]) {
            m_currentParticleData[i + 1]->setNumBlocks(numberOfBlocks);
            if (numberOfBlocks > 1) {
                m_currentParticleData[i + 1]->setBlock(rank());
            } else {
                m_currentParticleData[i + 1]->setBlock(0);
            }
            m_currentParticleData[i + 1]->setGrid(m_coordsOfParticles);
        }
    }
    addParticleToPorts();

    return true;
}

bool ReadCFX::addVolumeDataToPorts()
{
    //adds the 3d data to ports

    for (int portnum = 0; portnum < NumPorts; ++portnum) {
        if (m_currentVolumedata[portnum]) {
            updateMeta(m_currentVolumedata[portnum]);
            addObject(m_volumeDataOut[portnum], m_currentVolumedata[portnum]);
        }
    }
    return true;
}

bool ReadCFX::add2dDataToPorts()
{
    //adds the 2d data to ports

    for (int portnum = 0; portnum < Num2dPorts; ++portnum) {
        if (m_current2dData[portnum]) {
            updateMeta(m_current2dData[portnum]);
            addObject(m_2dDataOut[portnum], m_current2dData[portnum]);
        }
    }
    return true;
}

bool ReadCFX::addGridToPort(UnstructuredGrid::ptr grid)
{
    //adds the grid to the gridOut port

    if (grid) {
        //        Check which transformation matrix is added
        //        Matrix4 t = m_currentVolumedata[portnum]->getTransform();
        //        std::cerr << "Zone(data) = " << cfxExportZoneGet() << std::endl;
        //        std::cerr << m_currentVolumedata[portnum]->meta() << std::endl;
        //        for(int i=0;i<4;++i) {
        //            std::cerr << t(i,0) << " " << t(i,1) << " " << t(i,2) << " " << t(i,3) << std::endl;
        //        }
        updateMeta(grid);
        addObject(m_gridOut, grid);
    }
    return true;
}

bool ReadCFX::addPolygonToPort(Polygons::ptr polyg)
{
    //adds the polygon to the polygonOut port

    if (polyg) {
        updateMeta(polyg);
        addObject(m_polyOut, polyg);
    }
    return true;
}

bool ReadCFX::addParticleToPorts()
{
    //adds particles to port: one time port and NumParticlePorts data ports

    for (int portnum = 0; portnum <= NumParticlePorts; ++portnum) {
        if (m_currentParticleData[portnum]) {
            if (portnum == 0) {
                updateMeta(m_currentParticleData[portnum]);
                addObject(m_particleTime, m_currentParticleData[portnum]);
            } else {
                updateMeta(m_currentParticleData[portnum]);
                addObject(m_particleDataOut[portnum - 1], m_currentParticleData[portnum]);
            }
        }
    }
    return true;
}

Matrix4 ReadCFX::getTransformationMatrix(int zoneFlag, int timestep, bool setTransformation)
{
    // calculates the transformation matrix out of the rotation axis and angular velocity from cfxExportZoneIsRotating
    if (cfxExportZoneSet(zoneFlag, NULL) < 0) {
        std::cerr << "invalid zone number" << std::endl;
    }
    double rotAxis[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    double angularVel; //in radians per second
    if (!setTransformation ||
        !cfxExportZoneIsRotating(rotAxis, &angularVel)) //1 if zone is rotating, 0 if zone is not rotating
        return Matrix4::Identity();


    //    for(int i=0;i<3;++i) {
    //        std::cerr << "rotAxis[0][" << i << "] = " << rotAxis[0][i] << std::endl;
    //    }
    //    for(int i=0;i<3;++i) {
    //        std::cerr << "rotAxis[1][" << i << "] = " << rotAxis[1][i] << std::endl;
    //    }

    double startTime = cfxExportTimestepTimeGet(1);
    double currentTime = cfxExportTimestepTimeGet(timestep + 1);
    Scalar rot_angle = fmod((angularVel * (currentTime - startTime)), (2 * M_PI)); //in radians

    double x, y, z;
    x = rotAxis[1][0] - rotAxis[0][0];
    y = rotAxis[1][1] - rotAxis[0][1];
    z = rotAxis[1][2] - rotAxis[0][2];
    Vector3 rot_axis(x, y, z);
    rot_axis.normalize();

    Quaternion qrot(AngleAxis(rot_angle, rot_axis));
    Matrix4 rotMat(Matrix4::Identity());
    Matrix3 rotMat3(qrot.toRotationMatrix());
    rotMat.block<3, 3>(0, 0) = rotMat3;

    Vector3 translate(-rotAxis[0][0], -rotAxis[0][1], -rotAxis[0][2]);
    Matrix4 translateMat(Matrix4::Identity());
    translateMat.col(3) << translate, 1;

    Matrix4 transform(Matrix4::Identity());
    transform *= translateMat;
    transform *= rotMat;
    translate *= -1;
    translateMat.col(3) << translate, 1;
    transform *= translateMat;

    return transform;
}

void ReadCFX::setMeta(Object::ptr obj, int blockNr, int setMetaTimestep, int timestep, int numTimesteps,
                      index_t totalBlockNr, bool readTransientFile, Matrix4 transformMatrix)
{
    //sets the timestep, the number of timesteps, the real time, the block number and total number of blocks and the transformation matrix for a vistle object
    //timestep is -1 or if ntimesteps is 0 a variable is only in resfile

    if (!obj)
        return;

    if (m_ntimesteps == 0) {
        obj->setTimestep(-1);
        obj->setNumTimesteps(-1);
        obj->setRealTime(0);
    } else {
        if (timestep == -1 || m_lasttimestep->getValue() == -1) {
            obj->setNumTimesteps(-1);
            obj->setTimestep(-1);
        } else {
            if (readTransientFile) { //readTransientFile == 1 --> data belong to transient file
                obj->setRealTime(
                    cfxExportTimestepTimeGet(timestep + 1)); //+1 because cfxExport API's start with Index 1
            } else {
                obj->setRealTime(cfxExportTimestepTimeGet(m_lasttimestep->getValue() + 1) +
                                 cfxExportTimestepTimeGet(1)); //+1 because cfxExport API's start with Index 1
            }
            obj->setNumTimesteps(numTimesteps);
            obj->setTimestep(setMetaTimestep);
        }
    }
    obj->setBlock(blockNr);
    obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);
    obj->setTransform(transformMatrix);
}

bool ReadCFX::readTime(index_t numSel3dArea, index_t numSel2dArea, int setMetaTimestep, int timestep, int numTimesteps,
                       bool readTransientFile)
{
    //reads all information (3d and 2d data) for a given timestep and adds them to the ports

    for (index_t i = 0; i < numSel3dArea; ++i) {
        if (rankForVolumeAndTimestep(setMetaTimestep, i, numSel3dArea) == rank()) {
            vistle::UnstructuredGrid::ptr grid;
            if (!m_gridsInTimestep[i] || cfxExportGridChanged(m_previousTimestep, timestep + 1)) {
                m_gridsInTimestep[i] = loadGrid(i);
                if (!readTransientFile) {
                    m_gridsInTimestepForResfile[i] = m_gridsInTimestep[i]->clone();
                    Matrix4 t =
                        getTransformationMatrix(m_3dAreasSelected[i].zoneFlag, timestep, ignoreZoneMotionForGrid);
                    setMeta(m_gridsInTimestepForResfile[i], i, setMetaTimestep, -1, numTimesteps, numSel3dArea,
                            readTransientFile, t);
                }
                grid = m_gridsInTimestep[i];
            } else {
                grid = m_gridsInTimestep[i]->clone();
            }
            Matrix4 t = getTransformationMatrix(m_3dAreasSelected[i].zoneFlag, timestep, ignoreZoneMotionForGrid);
            setMeta(grid, i, setMetaTimestep, timestep, numTimesteps, numSel3dArea, readTransientFile, t);
            addGridToPort(grid);
            loadFields(grid, i, setMetaTimestep, timestep, numTimesteps, numSel3dArea, readTransientFile);
            addVolumeDataToPorts();
            grid.reset();
        }
    }

    for (index_t i = 0; i < numSel2dArea; ++i) {
        if (rankFor2dAreaAndTimestep(setMetaTimestep, i, numSel2dArea) == rank()) {
            vistle::Polygons::ptr polyg;
            if (!m_polygonsInTimestep[i] || cfxExportGridChanged(m_previousTimestep, timestep + 1)) {
                m_polygonsInTimestep[i] = loadPolygon(i);
                if (!readTransientFile) {
                    m_polygonsInTimestepForResfile[i] = m_polygonsInTimestep[i]->clone();
                    Matrix4 t = getTransformationMatrix(m_2dAreasSelected[i].idWithZone.zoneFlag, timestep,
                                                        ignoreZoneMotionForGrid);
                    setMeta(m_polygonsInTimestepForResfile[i], i, setMetaTimestep, -1, numTimesteps, numSel2dArea,
                            readTransientFile, t);
                }
                polyg = m_polygonsInTimestep[i];
            } else {
                polyg = m_polygonsInTimestep[i]->clone();
            }
            Matrix4 t =
                getTransformationMatrix(m_2dAreasSelected[i].idWithZone.zoneFlag, timestep, ignoreZoneMotionForGrid);
            setMeta(polyg, i, setMetaTimestep, timestep, numTimesteps, numSel2dArea, readTransientFile, t);
            addPolygonToPort(polyg);
            load2dFields(polyg, i, setMetaTimestep, timestep, numTimesteps, numSel2dArea, readTransientFile);
            add2dDataToPorts();
            polyg.reset();
        }
    }
    m_previousTimestep = timestep + 1;
    return true;
}


bool ReadCFX::prepare()
{
    //starts reading the .res file and loops than over all timesteps
#ifdef CFX_DEBUG
    std::cerr << "Compute Start. \n";
#endif

    index_t setMetaTimestep = 0;
    index_t firsttimestep = m_firsttimestep->getValue(), timeskip = m_timeskip->getValue();
    int lasttimestep = m_lasttimestep->getValue();
    index_t numTimesteps = ((lasttimestep - firsttimestep) / (timeskip + 1)) + 2;
    bool readTransientFile;

    if (!m_case.m_valid) {
        sendInfo("not a valid .res file entered: initialisation not yet done");
    } else {
        if (m_ExportDone) {
            initializeResultfile();
            m_case.parseResultfile();
        }
        //read variables out of .res file
        index_t numSel3dArea = collect3dAreas();
        index_t numSel2dArea = collect2dAreas();
        index_t numSelParticleTypes = collectParticles();
        m_gridsInTimestep.resize(numSel3dArea);
        m_gridsInTimestepForResfile.resize(numSel3dArea);
        m_polygonsInTimestep.resize(numSel2dArea);
        m_polygonsInTimestepForResfile.resize(numSel2dArea);
        readTransientFile = 0;
        readTime(numSel3dArea, numSel2dArea, numTimesteps - 1, 0, numTimesteps, readTransientFile);

        //read particles
        if (numSelParticleTypes > 0) {
            for (index_t i = 0; i < numSelParticleTypes; ++i) {
                loadParticles(m_particleTypesSelected[i]);
            }
        }

        //read variables out of timesteps .trn file
        if (m_ntimesteps != 0 && (lasttimestep != -1)) {
            readTransientFile = 1;
            for (int timestep = firsttimestep; timestep <= lasttimestep; timestep += (timeskip + 1)) {
                index_t timestepNumber = cfxExportTimestepNumGet(timestep + 1);
                if (cfxExportTimestepSet(timestepNumber) < 0) {
                    std::cerr << "cfxExportTimestepSet: invalid timestep number(" << timestepNumber << ")" << std::endl;
                } else {
                    m_case.parseResultfile();
                    numSel3dArea = collect3dAreas();
                    numSel2dArea = collect2dAreas();
                    readTime(numSel3dArea, numSel2dArea, setMetaTimestep, timestep, numTimesteps, readTransientFile);
                }
                setMetaTimestep++;
            }
            wrapCfxDone();
            m_ExportDone = true;
        }
    }

    m_fieldOut.clear();
    m_2dOut.clear();
    m_particleOut.clear();
    m_volumeDataOut.clear();
    m_2dDataOut.clear();
    m_particleDataOut.clear();
    m_gridsInTimestep.clear();
    m_gridsInTimestepForResfile.clear();
    m_polygonsInTimestep.clear();
    m_polygonsInTimestepForResfile.clear();
    m_coordsOfParticles.reset();
    m_currentVolumedata.clear();
    m_current2dData.clear();
    m_currentParticleData.clear();

    return true;
}
