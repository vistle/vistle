#ifndef READVISTLE_H
#define READVISTLE_H

#include <string.h>
#include "module.h"

#include "../WriteVistle/catalogue.h"

class ReadVistle: public vistle::Module {

 public:
   ReadVistle(const std::string &shmname, int rank, int size, int moduleID);
   ~ReadVistle();

 private:
   catalogue * readCatalogue(const int fd);
   vistle::Object::Info * readItemInfo(const int fd);
   vistle::Object::ptr readObject(const int fd, const vistle::Object::Info * info,
                               uint64_t start);

   vistle::Object::ptr load(const std::string & name);
   virtual bool compute();
};

#endif
