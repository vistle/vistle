#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include "module.h"

class IsoSurface: public vistle::Module {

 public:
   IsoSurface(int rank, int size, int moduleID);
   ~IsoSurface();

 private:
   vistle::Object * generateIsoSurface(const vistle::Object * grid,
                                       const vistle::Object * data,
                                       const vistle::Scalar isoValue);

   virtual bool compute();
};

#endif
