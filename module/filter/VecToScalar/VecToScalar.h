#ifndef VISTLE_VECTOSCALAR_VECTOSCALAR_H
#define VISTLE_VECTOSCALAR_VECTOSCALAR_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/vec.h>

class VecToScalar: public vistle::Module {
public:
    VecToScalar(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;
    vistle::IntParameter *m_caseParam;
};

#endif
