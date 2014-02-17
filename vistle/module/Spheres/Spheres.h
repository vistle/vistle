#ifndef TOSPHERES_H
#define TOSPHERES_H

#include <module/module.h>

class ToSpheres: public vistle::Module {

 public:
   ToSpheres(const std::string &shmname, int rank, int size, int moduleID);
   ~ToSpheres();

 private:
   virtual bool compute();

   vistle::FloatParameter *m_radius;
};

#endif
