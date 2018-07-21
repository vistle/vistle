#ifndef GENDAT_H
#define GENDAT_H

#include <module/module.h>

class Gendat: public vistle::Module {

 public:
   Gendat(const std::string &name, int moduleID, mpi::communicator comm);
   ~Gendat();

 private:
   virtual bool compute();
   void block(vistle::Index bx, vistle::Index by, vistle::Index bz, vistle::Index b, vistle::Index time);

   // parameters
   vistle::IntParameter *m_geoMode;
   vistle::IntParameter *m_dataModeScalar;
   vistle::IntParameter *m_dataMode[3];
   vistle::FloatParameter *m_dataScaleScalar;
   vistle::FloatParameter *m_dataScale[3];
   vistle::IntParameter *m_size[3];
   vistle::IntParameter *m_blocks[3];
   vistle::IntParameter *m_ghostLayerWidth;
   vistle::VectorParameter *m_min, *m_max;
   vistle::IntParameter *m_steps;
   vistle::IntParameter *m_animData;
};

#endif
