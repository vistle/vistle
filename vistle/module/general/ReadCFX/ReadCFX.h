#ifndef READCFX_H
#define READCFX_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/module.h>

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

   //Ports
   vistle::Port *m_gridOut;
   std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

   //CaseInfo m_case; caseinfos mithilfe export function in CFX bekommen

   std::vector<std::string> getFieldList() const;

   //! return MPI rank on which a block should be processed, takes OpenFOAM case, especially no. of blocks, into account
   int rankForBlock(int processor) const;
   /*vistle::Port *p_outPort1;        //mesh
   vistle::Port *p_outPort2;        //scalar data
   vistle::Port *p_outPort3;        //vector data
   vistle::Port *p_outPort4;        //region mesh
   vistle::Port *p_outPort5;        //region scalar data
   vistle::Port *p_outPort6;        //boundary
   vistle::Port *p_outPort7;        //boundary scalar data
   vistle::Port *p_outPort8;        //boundary vector data
   vistle::Port *p_outPort9;        //particle
   vistle::Port *p_outPort10;       //particle scalar data
   vistle::Port *p_outPort11;       //particle vector data*/

   //std::string _dir;
   //std::string _theFile;

   int i, n;
   int counts[cfxCNT_SIZE];
   int nzones, nnodes, nelems;
   int timeStepNum, iteration;

   cfxNode *nodes;
   cfxElement *elems;


   //int nscalars, nvectors, nregions, nparticleTracks, nparticleTypes;
   //int npscalars, npvectors, numTracks;

   //int level = 1, zone = 0, alias = 1, bndfix = 0, bnddat = 0;

   // number of digits in transient file suffix
   char zoneExt[256];


};

#endif
