#ifndef TESTINTERPOLATION_H
#define TESTINTERPOLATION_H

#include <module/module.h>

class TestInterpolation: public vistle::Module {

 public:
   TestInterpolation(const std::string &shmname, const std::string &name, int moduleID);
   ~TestInterpolation();

 private:
   virtual bool compute();

   vistle::IntParameter *m_count;
   vistle::IntParameter *m_createCelltree;
   vistle::IntParameter *m_mode;
};

#endif
