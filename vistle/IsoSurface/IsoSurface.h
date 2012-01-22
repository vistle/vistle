#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include "module.h"

class IsoSurface: public vistle::Module {

 public:
   IsoSurface(int rank, int size, int moduleID);
   ~IsoSurface();

 private:
   vistle::Triangles * generateIsoSurface(const vistle::UnstructuredGrid * grid,
                                          const vistle::Vec<float> * data);

   virtual bool compute();
};

#endif
