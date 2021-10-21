#ifndef TESTDYNAMICPORTS_H
#define TESTDYNAMICPORTS_H

#include <vistle/module/module.h>

class TestDynamicPorts: public vistle::Module {
public:
    TestDynamicPorts(const std::string &name, int moduleID, mpi::communicator comm);
    ~TestDynamicPorts();

    int m_numPorts;
    vistle::IntParameter *m_numPortsParam;

private:
    virtual bool compute() override;
    virtual bool changeParameter(const vistle::Parameter *param) override;
};

#endif
