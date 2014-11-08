#ifndef TOPOINTS_H
#define TOPOINTS_H

#include <module/module.h>

class ToPoints: public vistle::Module {

 public:
   ToPoints(const std::string &shmname, const std::string &name, int moduleID);
   ~ToPoints();

 private:
   virtual bool compute();
};

#endif
