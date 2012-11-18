#ifndef READVISTLE_H
#define READVISTLE_H

#include <string.h>
#include "module.h"

#include "../WriteVistle/catalogue.h"

class ReadArchive: public vistle::Module {

 public:
   ReadArchive(const std::string &shmname, int rank, int size, int moduleID);
   ~ReadArchive();

 private:
   bool load(const std::string & name);
   virtual bool compute();
   bool first_object;
};

#endif
