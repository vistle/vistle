#ifndef CUTGEOMETRY_H
#define CUTGEOMETRY_H

#include <module/module.h>
#include <core/vector.h>
#include "../IsoSurface/IsoDataFunctor.h"

class CutGeometry: public vistle::Module {

 public:
   CutGeometry(const std::string &shmname, const std::string &name, int moduleID);
   ~CutGeometry();

   vistle::Object::ptr cutGeometry(vistle::Object::const_ptr object) const;

 private:
   virtual bool compute() override;
   virtual bool changeParameter(const vistle::Parameter *param) override;
   IsoController isocontrol;
};

#endif
