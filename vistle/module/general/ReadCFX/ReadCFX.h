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
    IdWithZoneFlag(index_t r
                   , index_t z) {
        ID=r;
        zoneFlag=z;
    }

    index_t ID;
    index_t zoneFlag;
};

struct portData {
    portData() {

    }
    std::vector<vistle::DataBase::ptr> m_vectorResfileVolumeData;
    std::vector<std::int16_t> m_vectorVolumeDataVolumeNr;
};

struct boundaryPortData {
    boundaryPortData() {

    }
    std::vector<vistle::DataBase::ptr> m_vectorResfileBoundaryData;
    std::vector<std::int16_t> m_vectorBoundaryDataBoundaryNr;
};

class Variable {
public:
    Variable(std::string Name, int Dimension, int onlyMeaningful, int ID, int zone);
    std::string m_VarName;
    int m_VarDimension;
    int m_onlyMeaningfulOnBoundary; //if 1 than variable has only meaningful values on the boundary
    std::vector<IdWithZoneFlag> m_vectorIdwithZone;
};

class Boundary {
public:
    Boundary(std::string Name, int ID, int zone);
    std::string m_BoundName;
    std::vector<IdWithZoneFlag> m_vectorIdwithZone;
};

class CaseInfo {
public:
    CaseInfo();
    std::vector<std::string> m_field_param, m_boundary_param;
    bool m_valid;
    std::vector<Variable> m_allParam;
    std::vector<Boundary> m_allBoundaries;
    index_t m_numberOfVariables, m_numberOfBoundaries;

    bool checkFile(const char *filename);
    void parseResultfile();
    void getFieldList();
    std::vector<Variable> getCopyOfAllParam();
    std::vector<Boundary> getCopyOfAllBoundaries();
    index_t getNumberOfBoundaries();
};

class ReadCFX: public vistle::Module {
    static const int NumPorts = 3;
    static const int NumBoundaryPorts = 3;
    static const int correct = 1; //correct indicates whether to correct boundary node data according to the boundary condition (correct=1)
                                  //or not (correct=0) (result out of calculation), assuming that it exists.

 public:
   ReadCFX(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadCFX();
   virtual bool compute() override;
   static const int usr_level = 0; // Query the number of variables at interest level usr_level or below. If usr_level is 0, then the
                                   // total number of variables is returned.
   static const int alias = 1; // alias = 1 -> long variable name; alias = 0 -> short variable name


 private:
   bool changeParameter(const vistle::Parameter *p) override;
   bool m_ExportDone, m_addToPortResfileVolumeData, m_addToPortResfileBoundaryData;

   //Parameter
   vistle::StringParameter *m_resultfiledir, *m_zoneSelection, *m_boundarySelection;
   vistle::IntParameter *m_firsttimestep, *m_lasttimestep;
   vistle::IntParameter *m_timeskip;
   vistle::IntParameter *m_readBoundary; // *m_boundaryPatchesAsVariants;
   std::vector<vistle::StringParameter *> m_fieldOut, m_boundaryOut;
   vistle::coRestraint m_coRestraintZones, m_coRestraintBoundaries;

   index_t m_nzones, m_nvolumes, m_nnodes, m_ntimesteps, m_nregions; // m_nboundaries, m_nnodes, m_nelems, m_nvars, nscalars, nvectors, nparticleTracks, nparticleTypes
   int m_previousTimestep = 1;


   //Ports
   vistle::Port *m_gridOut, *m_polyOut;
   std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

   CaseInfo m_case;

   int counts[cfxCNT_SIZE];

   vistle::UnstructuredGrid::ptr grid;
   std::vector<IdWithZoneFlag> m_volumesSelected, m_boundariesSelected;
   std::map<int, std::map<int, vistle::DataBase::ptr>> m_currentVolumedata, m_currentBoundarydata;
   std::map<int, vistle::UnstructuredGrid::ptr> m_currentGrid;
   std::map<int, vistle::Polygons::ptr> m_currentPolygon;

   std::vector<portData> m_portDatas;
   std::vector<boundaryPortData> m_boundaryPortDatas;
   std::vector<vistle::UnstructuredGrid::ptr> m_vectorResfileGrid;
   std::vector<vistle::Polygons::ptr> m_vectorResfilePolygon;

   int rankForVolumeAndTimestep(int timestep, int volume, int numVolumes) const;
   int rankForBoundaryAndTimestep(int timestep, int boundary, int numBoundaries) const;
   vistle::UnstructuredGrid::ptr loadGrid(int volumeNr);
   vistle::Polygons::ptr loadPolygon(int boundaryNr);
   vistle::DataBase::ptr loadField(int volumeNr, Variable var);
   vistle::DataBase::ptr loadBoundaryField(int boundaryNr, Variable var);
   bool initializeResultfile();
   bool loadFields(int volumeNr, int processor, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes);
   bool loadBoundaryFields(int boundaryNr, int processor, int setMetaTimestep, int timestep, index_t numSelBoundaries, bool trnOrRes);
   index_t collectVolumes();
   index_t collectBoundaries();
   bool addVolumeDataToPorts(int processor);
   bool addBoundaryDataToPorts(int processor);
   bool addGridToPort(int volumeNr);
   bool addPolygonToPort(int boundaryNr);
   void setMeta(vistle::Object::ptr obj, int blockNr, int setMetaTimestep, int timestep, index_t totalBlockNr, bool trnOrRes);
   bool setDataObject(vistle::UnstructuredGrid::ptr grid, vistle::DataBase::ptr data, int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes);
   bool setBoundaryObject(vistle::Polygons::ptr grid, vistle::DataBase::ptr data, int volumeNr, int setMetaTimestep, int timestep, index_t numSelVolumes, bool trnOrRes);
   bool readTime(index_t numSelVolumes, index_t numSelBoundaries, int setMetaTimestep, int timestep, bool trnOrRes);
   bool clearResfileData ();


};

#endif
