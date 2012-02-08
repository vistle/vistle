#ifndef READFOAM_H
#define READFOAM_H

#include <vector>
#include <set>
#include <string.h>
#include "module.h"

class ReadFOAM: public vistle::Module {

 public:
   ReadFOAM(int rank, int size, int moduleID);
   ~ReadFOAM();

 private:
   std::vector<std::pair<std::string, vistle::Object *> > load(const std::string & casedir, const size_t partition);
   virtual bool compute();
};

#endif
