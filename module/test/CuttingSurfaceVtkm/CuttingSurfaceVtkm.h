#ifndef CUTTING_SURFACE_VTKM_H
#define CUTTING_SURFACE_VTKM_H

#include <vtkm/ImplicitFunction.h>

#include <vistle/module/module.h>
#include <vistle/vtkm/ImplFuncController.h>
#include <vistle/vtkm/VtkmModule.h>

class CuttingSurfaceVtkm: public VtkmModule {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceVtkm();

private:
    void runFilter(vtkm::cont::DataSet &inputData, vtkm::cont::DataSet &outputData) const override;
    bool changeParameter(const vistle::Parameter *param) override;

    vistle::IntParameter *m_computeNormals;

    ImplFuncController m_implFuncControl;
};

#endif // CUTTING_SURFACE_VTKM_H
