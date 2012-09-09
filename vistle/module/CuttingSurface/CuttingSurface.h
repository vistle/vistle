#ifndef CUTTINGSURFACE_H
#define CUTTINGSURFACE_H

#include "module.h"
#include "vector.h"

class CuttingSurface: public vistle::Module {

 public:
   CuttingSurface(int rank, int size, int moduleID);
   ~CuttingSurface();

 private:
   std::pair<vistle::Object *, vistle::Object *>
      generateCuttingSurface(const vistle::Object * grid,
                             const vistle::Object * data,
                             const vistle::Vector & normal,
                             const float distance);

   virtual bool compute();
};

#endif
