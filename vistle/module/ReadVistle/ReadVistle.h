#ifndef READVISTLE_H
#define READVISTLE_H

#include <string.h>
#include "module.h"

#include "../WriteVistle/catalogue.h"

class ReadVistle: public vistle::Module {

 public:
   ReadVistle(int rank, int size, int moduleID);
   ~ReadVistle();

 private:
   catalogue * readCatalogue(const int fd);
   iteminfo * readItemInfo(const int fd);
   vistle::Object * readObject(const int fd, const iteminfo * info,
                               uint64_t start);

   vistle::Object * load(const std::string & name);
   virtual bool compute();
};

#endif
