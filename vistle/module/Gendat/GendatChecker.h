#ifndef GENDATCHECKER_H
#define GENDATCHECKER_H

#include "module.h"

class GendatChecker: public vistle::Module {

 public:
   GendatChecker(int rank, int size, int moduleID);
   ~GendatChecker();

 private:
   virtual bool compute();
};

#endif
