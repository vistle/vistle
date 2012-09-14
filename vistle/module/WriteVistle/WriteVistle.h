#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <string.h>

#include <module.h>
#include <object.h>

#include "catalogue.h"

class WriteVistle: public vistle::Module {

 public:
   WriteVistle(int rank, int size, int moduleID);
   ~WriteVistle();

 private:
   vistle::Object::Info * createInfo(const vistle::Object * object, size_t offset);

   void createCatalogue(const vistle::Object * object, catalogue & c);
   void saveCatalogue(const int fd, const catalogue & c);
   void saveItemInfo(const int fd, const vistle::Object::Info * info);
   void saveObject(const int fd, const vistle::Object * object);

   void save(const std::string & name, vistle::Object * object);
   virtual bool compute();
};

#endif
