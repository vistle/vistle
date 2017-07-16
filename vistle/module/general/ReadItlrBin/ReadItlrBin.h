#ifndef READITLRBIN_H
#define READITLRBIN_H

#include <module/module.h>
#include <core/rectilineargrid.h>
#include <core/vec.h>
#include <string>
#include <vector>

class ReadItlrBin: public vistle::Module {

 public:
   ReadItlrBin(const std::string &name, int moduleID, mpi::communicator comm);
   ~ReadItlrBin();

private:
   static const int NumPorts = 3;

   bool changeParameter(const vistle::Parameter *p) override;
   bool compute() override;
   int rankForBlockAndTimestep(int block, int timestep);

   std::vector<vistle::RectilinearGrid::ptr> readGridBlocks(const std::string &filename, int npart=1);
   std::vector<std::string> readListFile(const std::string &filename) const;
   vistle::DataBase::ptr readFieldBlock(const std::string &filename, int part) const;

   vistle::StringParameter *m_gridFilename, *m_filename[NumPorts];
   vistle::IntParameter *m_numPartitions, *m_distributeTimesteps;
   vistle::IntParameter *m_firstStep, *m_lastStep, *m_stepSkip;
   vistle::Port *m_dataOut[NumPorts];

   int m_nparts;
   vistle::Index m_dims[3];

   struct Block {
       int part=0;
       int begin=0, end=0;
       int ghost[2]={0,0};
   };

   Block computeBlock(int part) const;
};
#endif

