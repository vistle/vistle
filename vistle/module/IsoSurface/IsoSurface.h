#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include <module/module.h>

class IsoSurface: public vistle::Module {

 public:
   IsoSurface(const std::string &shmname, int rank, int size, int moduleID);
   ~IsoSurface();

 private:
   std::pair<vistle::Object::ptr, vistle::Object::ptr> generateIsoSurface(
                                       vistle::Object::const_ptr grid,
                                       vistle::Object::const_ptr data,
                                       vistle::Object::const_ptr mapdata,
                                       const vistle::Scalar isoValue,
                                       int processorType
                                        );

   virtual bool compute();
   virtual bool prepare();
   virtual bool reduce(int timestep);

   vistle::FloatParameter *m_isovalue;
   vistle::StringParameter *m_shader, *m_shaderParams;
   vistle::IntParameter *m_processortype;

   vistle::Scalar min, max;
};

#endif
