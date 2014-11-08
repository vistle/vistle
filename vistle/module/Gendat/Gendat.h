#ifndef GENDAT_H
#define GENDAT_H

#include <module/module.h>

class Gendat: public vistle::Module {

 public:
   Gendat(const std::string &shmname, const std::string &name, int moduleID);
   ~Gendat();

 private:
   virtual bool compute();

   vistle::IntParameter *m_geoMode;
   vistle::IntParameter *m_dataMode;
};

#endif
