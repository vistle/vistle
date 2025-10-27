#ifndef VISTLE_DISPLACEVTKM_DISPLACEVTKM_H
#define VISTLE_DISPLACEVTKM_DISPLACEVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class DisplaceVtkm: public vistle::VtkmModule {
public:
    DisplaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~DisplaceVtkm();

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};

#endif // VISTLE_DISPLACEVTKM_DISPLACEVTKM_H
