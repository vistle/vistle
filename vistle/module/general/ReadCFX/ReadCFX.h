#ifndef READCFX_H
#define READCFX_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/module.h>

class ReadCFX: public vistle::Module {

 public:
   ReadCFX(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadCFX();

 private:

   bool parameterChanged(const vistle::Parameter *p);
   virtual bool compute();
   //int rankForBlock(int block) const;

   vistle::Integer m_firstBlock, m_lastBlock;
   vistle::Integer m_firstStep, m_lastStep, m_step;

   const char *resultfileName;

   std::string _dir;
   std::string _theFile;

   int i, n;
   int counts[cfxCNT_SIZE];
   int nscalars, nvectors, nregions, nzones, nparticleTracks, nparticleTypes;
   int npscalars, npvectors, numTracks;
   int timeStepNum, iteration;

   int level = 1, zone = 0, alias = 1, bndfix = 0, bnddat = 0;

   // number of digits in transient file suffix
   char zoneExt[256];


   /* CFX Variable
   char *pptr;
   char baseFileName[256], fileName[256], errmsg[256];
   int i, n, counts[cfxCNT_SIZE], dim, length, namelen;
   int nnodes, nelems, nscalars, nvectors, nvalues;
   int level = 1, zone = 0, alias = 1, bndfix = 0, bnddat = 0;
   int timestep = -1, infoOnly = 0;
   int ts, t, t1, t2;
   int nTimeDig = 1;
    // number of digits in transient file suffix
   char zoneExt[256];
    // zone extension added to the base filename
   int isTimestep = 0;
   float timeVal = 0.0; // time value in the single timestep mode
   char *wildcard = { "******" }; // used in transient file specificationFILE *fp;
   cfxNode *nodes;
   cfxElement *elems;
   float *var;
   */

};

#endif
