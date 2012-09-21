#ifndef CUTGEOMETRY_H
#define CUTGEOMETRY_H

#include "module.h"
#include "vector.h"

class CutGeometry: public vistle::Module {

 public:
   CutGeometry(int rank, int size, int moduleID);
   ~CutGeometry();

   vistle::Object::ptr cutGeometry(vistle::Object::const_ptr object,
                                const vistle::Vector & point,
                                const vistle::Vector & normal) const;

 private:
   virtual bool compute();
};

#endif
