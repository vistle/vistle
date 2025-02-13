#ifndef VISTLE_TESTINTERPOLATION_TESTINTERPOLATION_H
#define VISTLE_TESTINTERPOLATION_TESTINTERPOLATION_H

#include <vistle/module/module.h>

class TestInterpolation: public vistle::Module {
public:
    TestInterpolation(const std::string &name, int moduleID, mpi::communicator comm);
    ~TestInterpolation();

private:
    virtual bool compute();

    vistle::IntParameter *m_count;
    vistle::IntParameter *m_createCelltree;
    vistle::IntParameter *m_mode;
};

#endif
