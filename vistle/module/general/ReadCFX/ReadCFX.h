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
   bool ExportDone;

   //int rankForBlock(int block) const;

   //vistle::Integer m_firstBlock, m_lastBlock;
   //vistle::Integer m_firstStep, m_lastStep, m_step;


   //Parameter
   vistle::StringParameter *m_resultfiledir, *m_zoneSelection, *m_boundarySelection;
   vistle::IntParameter *m_firsttimestep, *m_lasttimestep;
   vistle::IntParameter *m_timeskip;
   vistle::IntParameter *m_readBoundary; // *m_boundaryPatchesAsVariants;
   std::vector<vistle::StringParameter *> m_fieldOut, m_boundaryOut;
   vistle::coRestraint m_coRestraintZones, m_coRestraintBoundaries;

   index_t m_nzones, m_nvolumes, m_nnodes, m_ntimesteps; // m_nboundaries, m_nregions, m_nnodes, m_nelems, m_nvars, nscalars, nvectors, nparticleTracks, nparticleTypes


   //Ports
   vistle::Port *m_gridOut;
   std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

   CaseInfo m_case;

   //cfxNode *nodeList;
   //cfxElement *elmList;
   // number of digits in transient file suffix
   //char zoneExt[256];
   int counts[cfxCNT_SIZE];

   vistle::UnstructuredGrid::ptr grid;
   std::vector<IdWithZoneFlag> m_volumesSelected, m_boundariesSelected;
   std::map<int, std::map<int, vistle::DataBase::ptr>> m_currentVolumedata;
   std::map<int, vistle::UnstructuredGrid::ptr>  m_currentGrid;

   int rankForVolumeAndTimestep(int timestep, int firsttimestep, int step, int volume, int numVolumes) const;
   int rankForBoundaryAndTimestep(int timestep, int firsttimestep, int step, int boundary, int numBoundaries) const;
   vistle::UnstructuredGrid::ptr loadGrid(int volumeNr);
   vistle::Polygons::ptr loadPolygon(int boundaryNr);
   vistle::DataBase::ptr loadField(int volumeNr, Variable var);
   vistle::DataBase::ptr loadBoundaryField(int boundaryNr, Variable var);
   bool loadFields(int volumeNr, int processor, int timestep, index_t numBlocks);
   bool loadBoundaryFields(int boundaryNr, int processor, int timestep, index_t numBlocks);
   index_t collectVolumes();
   index_t collectBoundaries();
   bool addVolumeDataToPorts(int processor);
   bool addGridToPort(int processor);
   void setMeta(vistle::Object::ptr obj, int volumeNr, int timestep, index_t numOfBlocks);


};

#endif
