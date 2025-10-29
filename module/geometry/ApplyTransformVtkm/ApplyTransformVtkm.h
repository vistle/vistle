#ifndef VISTLE_APPLYTRANSFORMVTKM_APPLYTRANSFORMVTKM_H
#define VISTLE_APPLYTRANSFORMVTKM_APPLYTRANSFORMVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class ApplyTransformVtkm: public vistle::VtkmModule {
public:
    ApplyTransformVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~ApplyTransformVtkm();

private:
    ModuleStatusPtr prepareInputGrid(const vistle::Object::const_ptr &grid,
                                     viskores::cont::DataSet &dataset) const override;
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
};

#endif
