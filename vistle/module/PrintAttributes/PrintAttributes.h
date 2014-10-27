#ifndef PRINTATTRIBUTES_H
#define PRINTATTRIBUTES_H

#include <module/module.h>

class PrintAttributes: public vistle::Module {

 public:
   PrintAttributes(const std::string &shmname, int rank, int size, int moduleID);
   ~PrintAttributes();

 private:
   virtual bool compute();
};

#endif
