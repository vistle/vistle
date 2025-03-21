#ifndef VISTLE_CLIPVTKM_CLIPVTKM_H
#define VISTLE_CLIPVTKM_CLIPVTKM_H

#include <vistle/vtkm/implicit_function_controller.h>
#include <vistle/vtkm/vtkm_module.h>

class ClipVtkm: public VtkmModule {
public:
    ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~ClipVtkm();

private:
    vistle::IntParameter *m_flip = nullptr;

    std::unique_ptr<vtkm::filter::Filter> setUpFilter() const override;

    bool changeParameter(const vistle::Parameter *param) override;

    ImplicitFunctionController m_implFuncControl;
};

#endif
