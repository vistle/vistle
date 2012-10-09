#ifndef READCOVISE_H
#define READCOVISE_H

#include <string.h>
#include "module.h"

class ReadCovise: public vistle::Module {

 public:
   ReadCovise(const std::string &shmname, int rank, int size, int moduleID);
   ~ReadCovise();

 private:
   bool readAttributes(const int fd, const bool byteswap);

   void setTimesteps(vistle::Object::ptr object, const int timestep);

   vistle::Object::ptr readSETELE(const int fd, const bool byteswap);
   vistle::Object::ptr readUNSGRD(const int fd, const bool byteswap);
   vistle::Object::ptr readUSTSDT(const int fd, const bool byteswap);
   vistle::Object::ptr readPOLYGN(const int fd, const bool byteswap);
   vistle::Object::ptr readGEOTEX(const int fd, const bool byteswap);
   vistle::Object::ptr readUSTVDT(const int fd, const bool byteswap);

   vistle::Object::ptr readObject(const int fd, bool byteswap);

   vistle::Object::const_ptr load(const std::string & filename);

   virtual bool compute();
};

#endif
