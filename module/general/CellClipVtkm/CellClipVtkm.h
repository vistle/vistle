#ifndef VISTLE_CELLCLIPVTKM_CELLCLIPVTKM_H
#define VISTLE_CELLCLIPVTKM_CELLCLIPVTKM_H

#include <vistle/vtkm/implicit_function_controller.h>
#include <vistle/vtkm/vtkm_module.h>

class CellClipVtkm: public vistle::VtkmModule {
public:
    CellClipVtkm(const std::string &name, int moduleID, mpi::communicator comm);

private:
    vistle::IntParameter *m_flip = nullptr;
    vistle::IntParameter *m_boundary = nullptr;

    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;

    bool changeParameter(const vistle::Parameter *param) override;

    vistle::ImplicitFunctionController m_implFuncControl;
};

#endif
