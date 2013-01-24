#ifndef CUTTINGSURFACE_H
#define CUTTINGSURFACE_H

#include <core/module.h>
#include <core/vector.h>

class CuttingSurface: public vistle::Module {

 public:
   CuttingSurface(const std::string &shmname, int rank, int size, int moduleID);
   ~CuttingSurface();

 private:
   std::pair<vistle::Object::ptr, vistle::Object::ptr>
      generateCuttingSurface(vistle::Object::const_ptr grid,
                             vistle::Object::const_ptr data,
                             const vistle::Vector & normal,
                             const vistle::Scalar distance);

   virtual bool compute();
};

#endif
