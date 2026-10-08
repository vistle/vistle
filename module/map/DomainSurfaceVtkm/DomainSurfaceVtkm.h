#ifndef VISTLE_DOMAINSURFACEVTKM_DOMAINSURFACEVTKM_H
#define VISTLE_DOMAINSURFACEVTKM_DOMAINSURFACEVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class DomainSurfaceVtkm: public vistle::VtkmModule {
public:
    DomainSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    ModuleStatusPtr prepareInputGrid(InputData &input) const override;

    std::unique_ptr<viskores::filter::Filter> setUpFilter(const VtkmModule::InputData &input) const override;
};

#endif
