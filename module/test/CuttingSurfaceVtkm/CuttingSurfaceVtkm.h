#ifndef VISTLE_CUTTINGSURFACEVTKM_CUTTINGSURFACEVTKM_H
#define VISTLE_CUTTINGSURFACEVTKM_CUTTINGSURFACEVTKM_H

#include <vistle/vtkm/ImplicitFunctionController.h>
#include <vistle/vtkm/VtkmModule.h>

class CuttingSurfaceVtkm: public VtkmModule {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceVtkm();

private:
    void runFilter(const vtkm::cont::DataSet &input, const std::string &fieldName,
                   vtkm::cont::DataSet &output) const override;
    bool changeParameter(const vistle::Parameter *param) override;

    vistle::IntParameter *m_computeNormals;

    ImplicitFunctionController m_implFuncControl;
};

#endif
