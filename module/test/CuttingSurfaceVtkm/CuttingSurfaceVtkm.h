#ifndef CUTTING_SURFACE_VTKM_H
#define CUTTING_SURFACE_VTKM_H

#include <vtkm/ImplicitFunction.h>

#include <vistle/module/module.h>
#include <vistle/vtkm/ImplFuncController.h>

class CuttingSurfaceVtkm: public vistle::Module {
public:
    CuttingSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~CuttingSurfaceVtkm();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool changeParameter(const vistle::Parameter *param) override;
    vistle::Object::ptr work(vistle::Object::const_ptr grid) const;

    vistle::Port *m_mapDataIn, *m_dataOut;

    vistle::IntParameter *m_computeNormals;

    ImplFuncController m_implFuncControl;
};

#endif // CUTTING_SURFACE_VTKM_H
