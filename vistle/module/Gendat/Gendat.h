#ifndef GENDAT_H
#define GENDAT_H

#include "module.h"

class Gendat: public vistle::Module {

 public:
   Gendat(const std::string &shmname, int rank, int size, int moduleID);
   ~Gendat();

 private:
   virtual bool compute();
};

#endif
