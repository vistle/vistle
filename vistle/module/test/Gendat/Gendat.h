#ifndef GENDAT_H
#define GENDAT_H

#include <module/module.h>

class Gendat: public vistle::Module {

 public:
   Gendat(const std::string &shmname, const std::string &name, int moduleID);
   ~Gendat();

 private:
   virtual bool compute();

   // parameters
   vistle::IntParameter *m_geoMode;
   vistle::IntParameter *m_dataMode;

   // parameter choice constants
   const int M_TRIANGLES = 0;
   const int M_POLYGONS = 1;
   const int M_UNIFORM = 2;
   const int M_RECTILINEAR = 3;
};

#endif
