#ifndef READCOVISE_H
#define READCOVISE_H

#include <string.h>
#include "module.h"

class ReadCovise: public vistle::Module {

 public:
   ReadCovise(int rank, int size, int moduleID);
   ~ReadCovise();

 private:
   void readAttributes(const int fd, const bool byteswap) const;
   void readSETELE(const int fd, std::vector<vistle::Object *> & objects,
                   const bool byteswap) const;
   void readUNSGRD(const int fd, std::vector<vistle::Object *> & objects,
                   bool byteswap, const int setElement = 0) const;
   void readUSTSDT(const int fd, std::vector<vistle::Object *> & objects,
                   bool byteswap, const int setElement = 0) const;

   void load(const std::string & name,
             std::vector<vistle::Object *> & objects) const;

   virtual bool compute();
};

#endif
