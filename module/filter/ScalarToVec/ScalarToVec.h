#ifndef SCALARTOVEC_H
#define SCALARTOVEC_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/vec.h>

class ScalarToVec: public vistle::Module {
public:
    ScalarToVec(const std::string &name, int moduleID, mpi::communicator comm);
    ~ScalarToVec();

private:
    static const int NumScalars = 3;

    virtual bool compute();

    vistle::Port *m_scalarIn[NumScalars];
    vistle::Port *m_vecOut;
};

#endif
