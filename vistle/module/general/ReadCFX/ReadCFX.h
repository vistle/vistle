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
#include <boost/bimap.hpp>


typedef vistle::Index index_t;
typedef vistle::Scalar scalar_t;
typedef boost::bimap<int, std::string> bm_type;

struct IdWithZoneFlag {

    IdWithZoneFlag(index_t r
                         , index_t z) {
        ID=r;
        zoneFlag=z;
    }

    index_t ID;
    index_t zoneFlag;
};

class CaseInfo {
public:
    CaseInfo();
    std::vector<std::string> m_field_param, m_boundary_param;
    bool m_valid;
    bm_type m_allParam;
    std::map<int, int> m_ParamDimension;
    index_t m_nvars;

    bool checkFile(const char *filename);
    void parseResultfile();
    void getFieldList();
};

class ReadCFX: public vistle::Module {
    static const int NumPorts = 3;
    static const int NumBoundaryPorts = 3;
    static const int correct = 1; //correct indicates whether to correct boundary node data according to the boundary condition (correct=1)
                                  //or not (correct=0) (result out of calculation), assuming that it exists.


 public:
   ReadCFX(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadCFX();
   virtual bool compute();
   static const int usr_level = 0; // Query the number of variables at interest level usr_level or below. If usr_level is 0, then the
                                   // total number of variables is returned.
   static const int alias = 1; // alias = 1 -> long variable name; alias = 0 -> short variable name


 private:
   bool parameterChanged(const vistle::Parameter *p);
   bool ExportDone;

   //int rankForBlock(int block) const;

   //vistle::Integer m_firstBlock, m_lastBlock;
   //vistle::Integer m_firstStep, m_lastStep, m_step;


   //Parameter
   vistle::StringParameter *m_resultfiledir, *m_zoneSelection, *m_boundarySelection;
   vistle::FloatParameter *m_starttime, *m_stoptime;
   vistle::IntParameter *m_timeskip;
   vistle::IntParameter *m_readBoundary; // *m_boundaryPatchesAsVariants;
   std::vector<vistle::StringParameter *> m_fieldOut, m_boundaryOut;
   vistle::coRestraint m_coRestraintZones, m_coRestraintBoundaries;

   index_t m_nzones, m_nvolumes, m_nboundaries; // m_nregions, m_nnodes, m_nelems, m_nvars, nscalars, nvectors, nparticleTracks, nparticleTypes


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
   std::vector<IdWithZoneFlag> m_volumesSelected;
   std::vector<std::int32_t> m_boundariesSelected;
   std::map<int, vistle::DataBase::ptr> m_currentVolumedata;
   std::map<int, vistle::DataBase::ptr> m_currentBoundaryVolumedata;
   std::map<int, vistle::UnstructuredGrid::ptr>  m_currentGrid;




   //! return MPI rank on which a block should be processed, takes OpenFOAM case, especially no. of blocks, into account
   int rankForBlock(int processor) const;
   vistle::UnstructuredGrid::ptr loadGrid(int volumeNr);
   vistle::Polygons::ptr loadPolygon(int boundaryNr);
   vistle::DataBase::ptr loadField(int volumeNr, int variableID);
   vistle::DataBase::ptr loadBoundaryField(int boundaryNr, int variableID);
   bool loadFields(int volumeNr);
   bool loadBoundaryFields(int boundaryNr);
   int collectVolumes();
   int collectBoundaries();
   bool addVolumeDataToPorts(int volumeNr);
   bool addGridToPort(int volumeNr);


};

#endif
