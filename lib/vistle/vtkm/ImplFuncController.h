#include <vtkm/ImplicitFunction.h>

#include <vistle/module/module.h>
#include <vistle/util/enum.h>

#include "export.h"

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(ImplFuncOption, (Plane)(Sphere)(CylinderX)(CylinderY)(CylinderZ)(Box))

class V_VTKM_EXPORT ImplFuncController {
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
