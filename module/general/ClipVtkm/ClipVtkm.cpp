#include <viskores/filter/contour/ClipWithImplicitFunction.h>

#include "ClipVtkm.h"

using namespace vistle;

MODULE_MAIN(ClipVtkm)

ClipVtkm::ClipVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3, false), m_implFuncControl(this)
{
    m_implFuncControl.init();
    m_flip = addIntParameter("flip", "flip inside out", false, Parameter::Boolean);
}

ClipVtkm::~ClipVtkm()
{}

std::unique_ptr<viskores::filter::Filter> ClipVtkm::setUpFilter() const
{
    auto filt = std::make_unique<viskores::filter::contour::ClipWithImplicitFunction>();
    filt->SetImplicitFunction(m_implFuncControl.function());
    filt->SetInvertClip(m_flip->getValue() != 0);
    return filt;
}

bool ClipVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}
