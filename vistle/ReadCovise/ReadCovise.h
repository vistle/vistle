#ifndef READCOVISE_H
#define READCOVISE_H

#include <string.h>
#include "module.h"

class ReadCovise: public vistle::Module {

 public:
   ReadCovise(int rank, int size, int moduleID);
   ~ReadCovise();

 private:
   void readAttributes(const int fd, const bool byteswap);
   vistle::Object * readSETELE(const int fd, const bool byteswap);
   vistle::Object * readUNSGRD(const int fd, bool byteswap);
   vistle::Object * readUSTSDT(const int fd, bool byteswap);
   vistle::Object * readPOLYGN(const int fd, bool byteswap);

   vistle::Object * load(const std::string & name);

   virtual bool compute();
};

#endif
