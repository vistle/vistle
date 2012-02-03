#ifndef CATALOGUE_H
#define CATALOGUE_H

#define __STDC_FORMAT_MACROS

#include <stdint.h>
#include <vector>

class header {

 public:
   header(unsigned char e, unsigned char ma, unsigned char mi,
          unsigned char pa)
      : endian(e), v_major(ma), v_minor(mi), v_patch(pa) {
   }
   header() { }
   unsigned char endian;
   unsigned char v_major;
   unsigned char v_minor;
   unsigned char v_patch;
};

class iteminfo {

 public:
   virtual ~iteminfo() {}
   uint64_t infosize;
   uint64_t itemsize;
   uint64_t offset;
   uint64_t type;
   uint64_t block;
   uint64_t timestep;
};

class setinfo: public iteminfo {

 public:
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
   iteminfo * item;
};

#endif
