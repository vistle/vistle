#ifndef VISTLE_CUTTINGSURFACEVTKM_CUTTINGSURFACEVTKM_H
#define VISTLE_CUTTINGSURFACEVTKM_CUTTINGSURFACEVTKM_H

#include <vistle/vtkm/implicit_function_controller.h>
#include <vistle/vtkm/vtkm_module.h>

class CuttingSurfaceVtkm: public vistle::VtkmModule {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;
    bool changeParameter(const vistle::Parameter *param) override;
    void setInputSpecies(const std::string &species) override;

    vistle::IntParameter *m_computeNormals;

    vistle::ImplicitFunctionController m_implFuncControl;
};

#endif
