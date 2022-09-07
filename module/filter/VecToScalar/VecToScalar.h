#ifndef VECTOSCALAR_H
#define VECTOSCALAR_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/vec.h>

class VecToScalar: public vistle::Module {
public:
    VecToScalar(const std::string &name, int moduleID, mpi::communicator comm);
    ~VecToScalar();

private:
    virtual bool compute();
    vistle::IntParameter *m_caseParam;
    vistle::Vec<vistle::Scalar>::ptr extract(vistle::Vec<vistle::Scalar, 3>::const_ptr &data,
                                             const vistle::Index &coord);
    vistle::Vec<vistle::Scalar>::ptr calculateAbsolute(vistle::Vec<vistle::Scalar, 3>::const_ptr &data);
};

#endif
