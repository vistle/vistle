#ifndef TOSPHERES_H
#define TOSPHERES_H

#include <module/module.h>

class ToTriangles: public vistle::Module {

 public:
   ToTriangles(const std::string &shmname, int rank, int size, int moduleID);
   ~ToTriangles();

 private:
   virtual bool compute();
};

#endif
