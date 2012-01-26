#ifndef CUTGEOMETRY_H
#define CUTGEOMETRY_H

#include "module.h"
#include "vector.h"

class CutGeometry: public vistle::Module {

 public:
   CutGeometry(int rank, int size, int moduleID);
   ~CutGeometry();

   vistle::Object * cutGeometry(const vistle::Object * object,
                                const vistle::util::Vector & point,
                                const vistle::util::Vector & normal) const;

 private:
   virtual bool compute();
};

#endif
