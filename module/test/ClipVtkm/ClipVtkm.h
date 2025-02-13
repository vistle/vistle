#ifndef VISTLE_CLIPVTKM_CLIPVTKM_H
#define VISTLE_CLIPVTKM_CLIPVTKM_H

#include <vistle/module/module.h>
#include <vtkm/ImplicitFunction.h>

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(ImplFuncOption, (Plane)(Sphere)(CylinderX)(CylinderY)(CylinderZ)(Box))

class ImplFuncController {
public:
    ImplFuncController(vistle::Module *module);
    void init();
    bool changeParameter(const vistle::Parameter *param);

    bool flip() const;
    vtkm::ImplicitFunctionGeneral func() const;

private:
    vistle::Module *m_module = nullptr;
    vistle::IntParameter *m_option = nullptr;
    vistle::IntParameter *m_flip = nullptr;
};

class ClipVtkm: public vistle::Module {
public:
    ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~ClipVtkm();

private:
    vistle::Port *m_dataOut = nullptr;

    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool changeParameter(const vistle::Parameter *param) override;

    ImplFuncController m_implFuncControl;
};

#endif
