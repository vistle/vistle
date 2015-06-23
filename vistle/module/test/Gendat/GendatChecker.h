#ifndef GENDATCHECKER_H
#define GENDATCHECKER_H

#include <module/module.h>

class GendatChecker: public vistle::Module {

 public:
   GendatChecker(const std::string &shmname, const std::string &name, int moduleID);
   ~GendatChecker();

 private:
   virtual bool compute();
};

#endif
