#ifndef WRITEVISTLE_H
#define WRITEVISTLE_H

#include <string.h>

#include <module.h>
#include <object.h>

class WriteArchive: public vistle::Module {

 public:
   WriteArchive(int rank, int size, int moduleID);

 private:
   void save(const std::string & name, vistle::Object * object);
   virtual bool compute();
};

#endif
