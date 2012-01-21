#ifndef READCOVISE_H
#define READCOVISE_H

#include <string.h>
#include "module.h"

class ReadCovise: public vistle::Module {

 public:
   ReadCovise(int rank, int size, int moduleID);
   ~ReadCovise();

 private:
   void readAttributes(const int fd);
   void readSETELE(const int fd,
                   std::vector<vistle::Object *> & objects);
   void readUNSGRD(const int fd,
                   std::vector<vistle::Object *> & objects);
   void load(const std::string & name,
             std::vector<vistle::Object *> & objects);

   virtual bool compute();
};

#endif
