#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include <module/module.h>

class IsoSurface: public vistle::Module {

 public:
   IsoSurface(const std::string &shmname, const std::string &name, int moduleID);
   ~IsoSurface();

 private:
   vistle::Object::ptr generateIsoSurface(vistle::Object::const_ptr grid,
                                       vistle::Object::const_ptr data,
                                       const vistle::Scalar isoValue);

   virtual bool compute();
   virtual bool prepare();
   virtual bool reduce(int timestep);

   vistle::FloatParameter *m_isovalue;
   vistle::StringParameter *m_shader, *m_shaderParams;

   vistle::Scalar min, max;
};

#endif
