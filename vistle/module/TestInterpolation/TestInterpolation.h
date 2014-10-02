#ifndef TESTINTERPOLATION_H
#define TESTINTERPOLATION_H

#include <module/module.h>

class TestInterpolation: public vistle::Module {

 public:
   TestInterpolation(const std::string &shmname, int rank, int size, int moduleID);
   ~TestInterpolation();

 private:
   virtual bool compute();

   vistle::IntParameter *m_count;
   vistle::IntParameter *m_createCelltree;
};

#endif
