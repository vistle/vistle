#ifndef READCFX_H
#define READCFX_H

#include <string>
#include <vector>
#include <utility> // std::pair
#include <util/coRestraint.h>
#include <core/unstr.h>
#include <core/index.h>
#include <map>

#include <util/sysdep.h>
#include <module/module.h>

#include <boost/config.hpp>
#include <boost/bimap.hpp>


typedef vistle::Index index_t;
typedef vistle::Scalar scalar_t;
typedef boost::bimap<int, std::string> bm_type;

struct VolumeIdWithZoneFlag {

    VolumeIdWithZoneFlag(index_t r
                         , index_t z) {
        volumeID=r;
        zoneFlag=z;
    }

    index_t volumeID;
    index_t zoneFlag;
};

class CaseInfo {
public:
    CaseInfo();
    std::vector<std::string> m_field_param, m_boundary_param;
    bool m_valid;
    bm_type m_allParam;
    std::map<int, int> m_ParamDimension;
    index_t nvars;

    bool checkFile(const char *filename);
    void getFieldList();
};

class ReadCFX: public vistle::Module {
    static const int NumPorts = 3;
    static const int NumBoundaryPorts = 3;

 public:
   ReadCFX(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadCFX();
   virtual bool compute();

 private:
   bool parameterChanged(const vistle::Parameter *p);

   //int rankForBlock(int block) const;

   //vistle::Integer m_firstBlock, m_lastBlock;
   //vistle::Integer m_firstStep, m_lastStep, m_step;


   //Parameter
   vistle::StringParameter *m_resultfiledir, *m_zoneSelection;
   vistle::FloatParameter *m_starttime, *m_stoptime;
   vistle::IntParameter *m_timeskip;
   vistle::IntParameter *m_readBoundary; // *m_boundaryPatchesAsVariants;
   std::vector<vistle::StringParameter *> m_fieldOut, m_boundaryOut;
   vistle::coRestraint m_zonesSelected;

   vistle::Index m_nelems, m_nzones, m_nnodes, m_nvolumes; // m_nregions, m_nvars, nscalars, nvectors, nparticleTracks, nparticleTypes


   //Ports
   vistle::Port *m_gridOut;
   std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

   CaseInfo m_case;

   //cfxNode *nodeList;
   //cfxElement *elmList;
   // number of digits in transient file suffix
   //char zoneExt[256];
   int counts[cfxCNT_SIZE];
   //std::string _dir;
   //std::string _theFile;

   vistle::UnstructuredGrid::ptr grid;
   std::vector<VolumeIdWithZoneFlag> m_selectedVolumes;
   std::map<int, vistle::DataBase::ptr> m_currentVolumedata;
   std::map<int, vistle::DataBase::ptr> m_currentBoundaryVolumedata;
   std::map<int, vistle::UnstructuredGrid::ptr>  m_currentGrid;




   //! return MPI rank on which a block should be processed, takes OpenFOAM case, especially no. of blocks, into account
   int rankForBlock(int processor) const;
   vistle::UnstructuredGrid::ptr loadGrid(int volumeNr);
   vistle::DataBase::ptr loadField(int volumeNr, int variableID);
   vistle::DataBase::ptr loadBoundaryField(int volumeNr, int variableID);
   bool loadFields(int volumeNr);
   int collectVolumes();
   bool addVolumeDataToPorts(int volumeNr);

};

#endif
