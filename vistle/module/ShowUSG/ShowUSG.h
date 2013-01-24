#ifndef SHOWUSG_H
#define SHOWUSG_H

#include <core/module.h>

class ShowUSG: public vistle::Module {

 public:
   ShowUSG(const std::string &shmname, int rank, int size, int moduleID);
   ~ShowUSG();

 private:
   virtual bool compute();
};

#endif
