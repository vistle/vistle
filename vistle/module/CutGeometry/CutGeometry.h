#ifndef CUTGEOMETRY_H
#define CUTGEOMETRY_H

#include <module/module.h>
#include <core/vector.h>

class CutGeometry: public vistle::Module {

 public:
   CutGeometry(const std::string &shmname, const std::string &name, int moduleID);
   ~CutGeometry();

   vistle::Object::ptr cutGeometry(vistle::Object::const_ptr object,
                                const vistle::Vector & point,
                                const vistle::Vector & normal) const;

 private:
   virtual bool compute();
};

#endif
