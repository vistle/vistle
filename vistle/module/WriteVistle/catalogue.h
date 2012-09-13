#ifndef CATALOGUE_H
#define CATALOGUE_H

#include <stdint.h>
#include <vector>

#include <object.h>

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

#if 0
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

class usginfo: public iteminfo {

 public:
   uint64_t numElements;
   uint64_t numCorners;
   uint64_t numVertices;
};

class datainfo: public iteminfo {

 public:
   uint64_t numElements;
};
#endif

class catalogue {

 public:
   uint64_t infosize;
   uint64_t itemsize;
   vistle::Object::Info * item;
};

#endif
