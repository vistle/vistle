#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <stdint.h>

#include <string.h>
#include "module.h"

class header {

 public:
   unsigned char endian;
   unsigned char major;
   unsigned char minor;
   unsigned char patch;
};

class iteminfo {

 public:
   virtual ~iteminfo() {}
   uint64_t infosize;
   uint64_t itemsize;
   uint64_t type;
   uint64_t block;
   uint64_t timestep;
};

class setinfo: public iteminfo {

 public:
   uint64_t settype;
   std::vector<iteminfo *> items;
};

class polygoninfo: public iteminfo {

 public:
   uint64_t numElements;
   uint64_t numCorners;
   uint64_t numVertices;
};

class catalogue {

 public:
   uint64_t infosize;
   uint64_t itemsize;
   std::vector<iteminfo *> items;
};

class WriteVistle: public vistle::Module {

 public:
   WriteVistle(int rank, int size, int moduleID);
   ~WriteVistle();

 private:
   iteminfo * createInfo(vistle::Object * object);
   void createCatalogue(const vistle::Object * object, catalogue & c);
   void save(const std::string & name, vistle::Object * object);
   virtual bool compute();
};

#endif
