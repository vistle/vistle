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
   void parseBoundary(const std::string & casedir, const int partition);
   std::vector<std::pair<std::string, vistle::Object::ptr> > load(const std::string & casedir, const size_t partition);
   virtual bool compute();
};

#endif
