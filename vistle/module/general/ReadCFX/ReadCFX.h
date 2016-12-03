#ifndef READCFX_H
#define READCFX_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/module.h>

class CaseInfo {
public:
    CaseInfo(bool m_valid = false);
    ~CaseInfo();
    std::map<int, std::string> m_field_param, m_boundary_param;
    bool m_valid;

    void getCaseInfo(const std::string &resultfiledir);
    int checkFile(const char *filename);
    void checkFields(std::map<int, std::string> &field_param, std::map<int, std::string> &boundary_param);
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
   vistle::StringParameter *m_resultfiledir;
   vistle::FloatParameter *m_starttime, *m_stoptime;
   vistle::IntParameter *m_timeskip;
   vistle::IntParameter *m_readGrid, *m_readBoundary; // *m_boundaryPatchesAsVariants;
   std::vector<vistle::StringParameter *> m_fieldOut, m_boundaryOut;

   int nscalars, nvectors, nregions, nelems, nzones, nparticleTracks, nparticleTypes;
   int nnodes;

   //Ports
   vistle::Port *m_gridOut;
   std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

   CaseInfo m_case;

   std::vector<std::string> getFieldList() const;

   //! return MPI rank on which a block should be processed, takes OpenFOAM case, especially no. of blocks, into account
   int rankForBlock(int processor) const;

   //std::string _dir;
   //std::string _theFile;

   int counts[cfxCNT_SIZE];
   int timeStepNum, iteration;

   cfxNode *nodes;
   cfxElement *elems;

   //int npscalars, npvectors, numTracks;

   //int level = 1, zone = 0, alias = 1, bndfix = 0, bnddat = 0;

   // number of digits in transient file suffix
   char zoneExt[256];


};

#endif
