#ifndef READCFX_H
#define READCFX_H

#include <string>
#include <vector>
#include <utility> // std::pair
#include <util/coRestraint.h>
#include <core/unstr.h>
#include <core/index.h>
#include <core/polygons.h>
#include <map>

#include <util/sysdep.h>
#include <module/module.h>

#include <boost/config.hpp>

typedef vistle::Index index_t;
typedef vistle::Scalar scalar_t;

struct IdWithZoneFlag {
    IdWithZoneFlag();
    IdWithZoneFlag(index_t r, index_t z);
    index_t ID;
    index_t zoneFlag;
};

struct PortData {
    std::vector<vistle::DataBase::ptr> vectorResfileVolumeData;
    std::vector<std::int16_t> vectorVolumeDataVolumeNr;
};

struct Area2dPortData {
    std::vector<vistle::DataBase::ptr> resfile2dDataVec;
    std::vector<std::int16_t> resfile2dIdVec;
};

struct Areas2d {
    IdWithZoneFlag idWithZone;
    std::string area2dType;
};

struct Variable {
    Variable(std::string Name, int Dimension, int onlyMeaningful, int ID, int zone);
    std::string varName;
    int varDimension;
    int onlyMeaningfulOnBoundary; //if 1 than variable has only meaningful values on the boundary
    std::vector<IdWithZoneFlag> vectorIdwithZone;
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
    std::vector<std::string> m_field_param, m_boundary_param, m_region_param;
    bool m_valid;
    std::vector<Variable> m_allParam;
    std::vector<Boundary> m_allBoundaries;
    std::vector<Region> m_allRegions;
    index_t m_numberOfVariables, m_numberOfBoundaries, m_numberOfRegions;

    bool checkFile(const char *filename);
    void parseResultfile();
    void getFieldList();
    std::vector<Variable> getCopyOfAllParam();
    std::vector<Boundary> getCopyOfAllBoundaries();
    index_t getNumberOfBoundaries();
    std::vector<Region> getCopyOfAllRegions();
    index_t getNumberOfRegions();
};

class ReadCFX: public vistle::Module {
    static const int NumPorts = 3;
    static const int Num2dPorts = 3;
    static const int correct = 1; //correct indicates whether to correct boundary node data according to the boundary condition (correct=1)
                                  //or not (correct=0) (result out of calculation), assuming that it exists.

 public:
   ReadCFX(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadCFX();
   virtual bool compute() override;
   static const int usr_level = 0; // Query the number of variables at interest level usr_level or below. If usr_level is 0, then the
                                   // total number of variables is returned.
   static const int alias = 0; // alias = 1 -> long variable name; alias = 0 -> short variable name


 private:
   bool changeParameter(const vistle::Parameter *p) override;
   bool m_ExportDone, m_addToPortResfileVolumeData, m_addToPortResfile2dData;

   //Parameter
   vistle::StringParameter *m_resultfiledir, *m_zoneSelection, *m_2dAreaSelection;
   vistle::IntParameter *m_firsttimestep, *m_lasttimestep;
   vistle::IntParameter *m_timeskip;
   vistle::IntParameter *m_readBoundary; // *m_boundaryPatchesAsVariants;
   std::vector<vistle::StringParameter *> m_fieldOut, m_2dOut;
   vistle::coRestraint m_coRestraintZones, m_coRestraint2dAreas;

   index_t m_nzones, m_nvolumes, m_nnodes, m_ntimesteps, m_nregions; // m_nboundaries, m_nnodes, m_nelems, m_nvars, nscalars, nvectors, nparticleTracks, nparticleTypes
   int m_previousTimestep = 1;

   //Ports
   vistle::Port *m_gridOut, *m_polyOut;
   std::vector<vistle::Port *> m_volumeDataOut, m_2dDataOut;

   //Data
   CaseInfo m_case;
   int counts[cfxCNT_SIZE];
   vistle::UnstructuredGrid::ptr grid;
   std::vector<IdWithZoneFlag> m_volumesSelected;
   std::vector<Areas2d> m_2dAreasSelected;
   std::vector<vistle::UnstructuredGrid::ptr> m_gridsInTimestep;
   std::vector<vistle::Polygons::ptr> m_polygonsInTimestep;
   std::vector<vistle::DataBase::ptr> m_currentVolumedata, m_current2dData;
   std::vector<PortData> m_portDatas;
   std::vector<Area2dPortData> m_2dPortDatas;
   std::vector<vistle::UnstructuredGrid::ptr> m_ResfileGridVec;
   std::vector<vistle::Polygons::ptr> m_ResfilePolygonVec;

   int rankForVolumeAndTimestep(int timestep, int volume, int numVolumes) const;
   int rankFor2dAreaAndTimestep(int timestep, int area2d, int num2dAreas) const;
   vistle::UnstructuredGrid::ptr loadGrid(int volumeNr);
   vistle::Polygons::ptr loadPolygon(int area2d);
   vistle::DataBase::ptr loadField(int volumeNr, Variable var);
   vistle::DataBase::ptr load2dField(int area2d, Variable var);
   bool initializeResultfile();
   bool loadFields(int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes);
   bool load2dFields(int area2d, int setMetaTimestep, int timestep, index_t numSel2dArea, bool trnOrRes);
   index_t collectVolumes();
   index_t collect2dAreas();
   bool addVolumeDataToPorts();
   bool add2dDataToPorts();
   bool addGridToPort(int volumeNr);
   bool addPolygonToPort(int area2d);
   void setMeta(vistle::Object::ptr obj, int blockNr, int setMetaTimestep, int timestep, index_t totalBlockNr, bool trnOrRes);
   bool setDataObject(vistle::UnstructuredGrid::ptr grid, vistle::DataBase::ptr data, int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes);
   bool set2dObject(vistle::Polygons::ptr grid, vistle::DataBase::ptr data, int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes);
   bool readTime(index_t numSelVolumes, index_t numSel2dArea, int setMetaTimestep, int timestep, bool trnOrRes);
   bool clearResfileData ();
   bool free2dArea(const char *area2dType, int area2d);

};

#endif
