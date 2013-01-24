#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include <core/module.h>

class IsoSurface: public vistle::Module {

 public:
   IsoSurface(const std::string &shmname, int rank, int size, int moduleID);
   ~IsoSurface();

 private:
   vistle::Object::ptr generateIsoSurface(vistle::Object::const_ptr grid,
                                       vistle::Object::const_ptr data,
                                       const vistle::Scalar isoValue);

   virtual bool compute();
};

#endif
