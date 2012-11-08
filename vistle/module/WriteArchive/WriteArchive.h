#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <string.h>

#include <module.h>
#include <object.h>

class WriteArchive: public vistle::Module {

 public:
   WriteArchive(const std::string &shmname, int rank, int size, int moduleID);

 private:
   virtual bool compute();
};

#endif
