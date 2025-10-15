#ifndef VISTLE_PASSTHROUGHVTKM_PASSTHROUGHVTKM_H
#define VISTLE_PASSTHROUGHVTKM_PASSTHROUGHVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class PassThroughVtkm: public vistle::VtkmModule {
public:
    PassThroughVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};
#endif
