#ifndef TESTDYNAMICPORTS_H
#define TESTDYNAMICPORTS_H

#include <module/module.h>

class TestDynamicPorts: public vistle::Module {

 public:
   TestDynamicPorts(const std::string &shmname, const std::string &name, int moduleID);
   ~TestDynamicPorts();

   int m_numPorts;
   vistle::IntParameter *m_numPortsParam;

 private:
   virtual bool compute() override;
   virtual bool changeParameter(const vistle::Parameter *param) override;
};

#endif
