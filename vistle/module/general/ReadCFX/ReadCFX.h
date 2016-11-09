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
};

#endif
