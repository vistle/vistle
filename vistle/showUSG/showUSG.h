#ifndef SHOWUSG_H
#define SHOWUSG_H

#include "module.h"

class ShowUSG: public vistle::Module {

 public:
   ShowUSG(int rank, int size, int moduleID);
   ~ShowUSG();

 private:
   virtual bool compute();
};

#endif
