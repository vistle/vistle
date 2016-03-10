#ifndef READMODEL_H
#define READMODEL_H

#include <string>
#include <vector>
#include <utility> // std::pair

#include <util/sysdep.h>
#include <module/module.h>

class ReadModel: public vistle::Module {

 public:
   ReadModel(const std::string &shmname, const std::string &name, int moduleID);
   ~ReadModel();

 private:

   vistle::Object::ptr load(const std::string & filename);

   virtual bool compute();
   int rankForBlock(int block) const;

   int m_firstBlock, m_lastBlock;
   int m_firstStep, m_lastStep, m_step;
};

#endif
