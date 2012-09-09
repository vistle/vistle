#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <string.h>
#include "module.h"

#include "catalogue.h"

class WriteVistle: public vistle::Module {

 public:
   WriteVistle(int rank, int size, int moduleID);
   ~WriteVistle();

 private:
   iteminfo * createInfo(vistle::Object * object, size_t offset);

   void createCatalogue(const vistle::Object * object, catalogue & c);
   void saveCatalogue(const int fd, const catalogue & c);
   void saveItemInfo(const int fd, const iteminfo * info);
   void saveObject(const int fd, const vistle::Object * object);

   void save(const std::string & name, vistle::Object * object);
   virtual bool compute();
};

#endif
