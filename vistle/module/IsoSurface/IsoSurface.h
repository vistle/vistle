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

   virtual bool compute() override;
   virtual bool prepare() override;
   virtual bool reduce(int timestep) override;
   bool parameterChanged(const vistle::Parameter *param) override;



   vistle::FloatParameter *m_isovalue;
   vistle::IntParameter *m_processortype;
   vistle::IntParameter *m_option;
   vistle::Port *m_mapDataIn, *m_mapDataOut;

   vistle::Scalar m_min, m_max;
};

#endif
