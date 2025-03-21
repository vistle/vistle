#include <vtkm/ImplicitFunction.h>

#include <vistle/module/module.h>
#include <vistle/util/enum.h>

#include "export.h"

// order has to match OpenCOVER's CuttingSurfaceInteraction
DEFINE_ENUM_WITH_STRING_CONVERSIONS(ImplFuncOption, (Plane)(Sphere)(CylinderX)(CylinderY)(CylinderZ)(Box))

/*
    This class creates implicit functions for VTK-m filters based on module parameters
    which it creates and handles.
*/
class V_VTKM_EXPORT ImplicitFunctionController {
public:
    ImplicitFunctionController(vistle::Module *module);
    /*
        Creates the necessary module parameters for the implicit function.
        Call this in the constructor of the VTK-m module.
    */
    void init();
    /*
        Updates the implicit function when a module parameter is changed.
        Call this in the changeParameter method of the VTK-m module.
    */
    bool changeParameter(const vistle::Parameter *param);

    /*
        Returns the implicit function based on the current module parameters.
    */
    vtkm::ImplicitFunctionGeneral function() const;

private:
    vistle::Module *m_module = nullptr;
    vistle::IntParameter *m_option = nullptr;
};
