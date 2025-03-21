#ifndef VISTLE_CUTTINGSURFACEVTKM_CUTTINGSURFACEVTKM_H
#define VISTLE_CUTTINGSURFACEVTKM_CUTTINGSURFACEVTKM_H

#include <vistle/vtkm/implicit_function_controller.h>
#include <vistle/vtkm/vtkm_module.h>

class CuttingSurfaceVtkm: public VtkmModule {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceVtkm();

private:
    std::unique_ptr<vtkm::filter::Filter> setUpFilter() const override;
    bool changeParameter(const vistle::Parameter *param) override;

    vistle::IntParameter *m_computeNormals;

    ImplicitFunctionController m_implFuncControl;
};

#endif
