#ifndef VISTLE_READCFX_READCFX_H
#define VISTLE_READCFX_READCFX_H

#include <string>
#include <vector>
#include <utility> // std::pair
#include <vistle/util/coRestraint.h>
#include <vistle/core/unstr.h>
#include <vistle/core/index.h>
#include <vistle/core/polygons.h>
//#include <map>
#include <vistle/core/lines.h>

#include <vistle/util/sysdep.h>
#include <vistle/module/module.h>

#include <boost/config.hpp>

typedef vistle::Index index_t;
typedef vistle::Scalar scalar_t;

struct IdWithZoneFlag {
    IdWithZoneFlag();
    IdWithZoneFlag(index_t r, index_t z);
    index_t ID;
    index_t zoneFlag;
};

struct Areas2d {
    IdWithZoneFlag idWithZone;
    bool boundary;
};

struct Variable {
    Variable(std::string Name, int Dimension, int onlyMeaningful, int ID, int zone);
    std::string varName;
    int varDimension;
    int onlyMeaningfulOnBoundary; //if 1 than variable has only meaningful values on the boundary
    std::vector<IdWithZoneFlag> vectorIdwithZone;
};

struct Particle {
    Particle(std::string Type, std::string Name, int Dimension, int ID, int zone);
    std::string particleType;
    std::string varName;
    int varDimension;
    IdWithZoneFlag idWithZone;
};

struct Boundary {
    Boundary(std::string Name, int ID, int zone);
    std::string boundName;
    IdWithZoneFlag idWithZone;
};

struct Region {
    Region(std::string Name, int ID, int zone);
    std::string regionName;
    IdWithZoneFlag idWithZone;
};

class CaseInfo {
public:
    CaseInfo();
    std::vector<std::string> m_field_param, m_boundary_param, m_region_param, m_particle_types;
    std::vector<std::string> m_variableInTransientFile;
    bool m_valid;
    std::vector<Variable> m_allParam;
    std::vector<Particle> m_allParticle;
    std::vector<Boundary> m_allBoundaries;
    std::vector<Region> m_allRegions;
    index_t m_numberOfVariables, m_numberOfBoundaries, m_numberOfRegions, m_numberOfParticles;

    bool checkFile(const char *filename);
    void parseResultfile();
    void getFieldList();
    std::vector<Variable> getCopyOfAllParam();
    std::vector<Boundary> getCopyOfAllBoundaries();
    index_t getNumberOfBoundaries();
    std::vector<Region> getCopyOfAllRegions();
    std::vector<Particle> getCopyOfAllParticles();
    std::vector<std::string> getCopyOfTrnVars();
    index_t getNumberOfRegions();
    bool checkWhichVariablesAreInTransientFile(index_t ntimesteps);
};

class ReadCFX: public vistle::Module {
    static const int NumPorts = 3;
    static const int Num2dPorts = 3;
    static const int NumParticlePorts = 3;
    static const int correct =
        1; //correct indicates whether to correct boundary node data according to the boundary condition (correct=1)
    //or not (correct=0) (result out of calculation), assuming that it exists.

public:
    ReadCFX(const std::string &name, int moduleID, mpi::communicator comm);

    virtual bool prepare() override;
    static const int usr_level =
        0; // Query the number of variables at interest level usr_level or below. If usr_level is 0, then the
    // total number of variables is returned.
    static const int alias = 0; // alias = 1 -> long variable name; alias = 0 -> short variable name


private:
    bool changeParameter(const vistle::Parameter *p) override;
    bool m_ExportDone = true; // m_addToPortResfileVolumeData, m_addToPortResfile2dData;

    //Parameter
    vistle::StringParameter *m_resultfiledir, *m_zoneSelection, *m_2dAreaSelection, *m_particleSelection;
    vistle::IntParameter *m_firsttimestep, *m_lasttimestep;
    vistle::IntParameter *m_timeskip;
    vistle::IntParameter *m_readDataTransformed, *m_readGridTransformed;
    std::vector<vistle::StringParameter *> m_fieldOut, m_2dOut, m_particleOut;
    vistle::coRestraint m_coRestraintZones, m_coRestraint2dAreas, m_coRestraintParticle;

    index_t m_nzones, m_nvolumes, m_nnodes, m_ntimesteps, m_nregions; // m_nboundaries, m_nnodes, m_nelems, m_nvars
    int m_previousTimestep = 1;
    int ignoreZoneMotionForData, ignoreZoneMotionForGrid; //cfxMOTION_USE = 0, cfxMOTION_IGNORE = 1

    //Ports
    vistle::Port *m_gridOut, *m_polyOut, *m_particleTime;
    std::vector<vistle::Port *> m_volumeDataOut, m_2dDataOut, m_particleDataOut;

    //Data
    CaseInfo m_case;
    int counts[cfxCNT_SIZE];
    std::vector<IdWithZoneFlag> m_3dAreasSelected;
    std::vector<Areas2d> m_2dAreasSelected;
    std::vector<std::int32_t> m_particleTypesSelected;
    std::vector<vistle::UnstructuredGrid::ptr> m_gridsInTimestep, m_gridsInTimestepForResfile;
    std::vector<vistle::Polygons::ptr> m_polygonsInTimestep, m_polygonsInTimestepForResfile;
    vistle::Lines::ptr m_coordsOfParticles;
    std::vector<vistle::DataBase::ptr> m_currentVolumedata, m_current2dData, m_currentParticleData;

    int rankForVolumeAndTimestep(int setMetaTimestep, int volume, int numVolumes) const;
    int rankFor2dAreaAndTimestep(int setMetaTimestep, int area2d, int num2dAreas) const;
    int trackStartandEndForRank(int rank, int *firstTrackForRank, int *lastTrackForRank, int numberOfTracks);
    vistle::UnstructuredGrid::ptr loadGrid(int area3d);
    vistle::Polygons::ptr loadPolygon(int area2d);
    vistle::Lines::ptr loadParticleTrackCoords(int particleTypeNumber, const index_t numVertices, int firstTrackForRank,
                                               int lastTrackForRank);
    vistle::DataBase::ptr loadField(int area3d, Variable var);
    vistle::DataBase::ptr load2dField(int area2d, Variable var);
    vistle::DataBase::ptr loadParticleTime(int particleTypeNumber, const index_t NumVertices, int firstTrackForRank,
                                           int lastTrackForRank);
    vistle::DataBase::ptr loadParticleValues(int particleTypeNumber, Particle particle, const index_t NumVertices,
                                             int firstTrackForRank, int lastTrackForRank);
    bool initializeResultfile();
    bool loadFields(vistle::UnstructuredGrid::ptr grid, int area3d, int setMetaTimestep, int timestep, int numTimesteps,
                    index_t numSel3dArea, bool readTransientFile);
    bool load2dFields(vistle::Polygons::ptr polyg, int area2d, int setMetaTimestep, int timestep, int numTimesteps,
                      index_t numSel2dArea, bool readTransientFile);
    bool loadParticles(int particleTypeNumber);
    index_t collect3dAreas();
    index_t collect2dAreas();
    index_t collectParticles();
    bool addVolumeDataToPorts();
    bool add2dDataToPorts();
    bool addGridToPort(vistle::UnstructuredGrid::ptr grid);
    bool addPolygonToPort(vistle::Polygons::ptr polyg);
    bool addParticleToPorts();
    vistle::Matrix4 getTransformationMatrix(int zoneFlag, int timestep, bool setTransformation);
    void setMeta(vistle::Object::ptr obj, int blockNr, int setMetaTimestep, int timestep, int numTimesteps,
                 index_t totalBlockNr, bool readTransientFile, vistle::Matrix4 transformMatrix);
    bool readTime(index_t numSel3dArea, index_t numSel2dArea, int setMetaTimestep, int timestep, int numTimesteps,
                  bool readTransientFile);
    bool free2dArea(bool boundary, int area2d);
};

#endif
