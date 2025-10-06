#ifndef VISTLE_VECTOSCALARVTKM_VECTOSCALARVTKM_H
#define VISTLE_VECTOSCALARVTKM_VECTOSCALARVTKM_H

#include <vistle/vtkm/vtkm_module.h>


class VecToScalarVtkm: public vistle::VtkmModule {
public:
    VecToScalarVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~VecToScalarVtkm();

private:

    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};

#endif



