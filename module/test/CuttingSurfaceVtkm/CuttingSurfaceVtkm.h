#ifndef CUTTING_SURFACE_VTKM_H
#define CUTTING_SURFACE_VTKM_H

#include <vistle/vtkm/ImplicitFunctionController.h>
#include <vistle/vtkm/VtkmModule.h>

class CuttingSurfaceVtkm: public VtkmModule {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceVtkm();

private:
    void runFilter(vtkm::cont::DataSet &filterInput, vtkm::cont::DataSet &filterOutput) const override;
    bool changeParameter(const vistle::Parameter *param) override;

    vistle::IntParameter *m_computeNormals;

    ImplicitFunctionController m_implFuncControl;
};

#endif // CUTTING_SURFACE_VTKM_H
