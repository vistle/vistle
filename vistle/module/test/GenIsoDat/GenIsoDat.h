#ifndef GENISODAT_H
#define GENISODAT_H

#include <module/module.h>

class GenIsoDat: public vistle::Module {

 public:
   GenIsoDat(const std::string &shmname, const std::string &name, int moduleID);
   ~GenIsoDat();

 private:
   virtual bool compute();

   vistle::IntParameter *m_cellTypeParam;
   vistle::IntParameter *m_caseNumParam;
};

#endif
