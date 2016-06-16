#ifndef FLATTENTRIANGLES_H
#define FLATTENTRIANGLES_H

#include <module/module.h>

class FlattenTriangles: public vistle::Module {

 public:
   FlattenTriangles(const std::string &shmname, const std::string &name, int moduleID);
   ~FlattenTriangles();

 private:
   virtual bool compute();
};

#endif
