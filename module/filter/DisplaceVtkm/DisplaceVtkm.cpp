#include "filter/DisplaceFilter.h"

#include "DisplaceVtkm.h"

MODULE_MAIN(DisplaceVtkm)

using namespace vistle;

DisplaceVtkm::DisplaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Require)
{
    p_component =
        addIntParameter("component", "component to displace for scalar input", DisplaceComponent::Z, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(p_component, DisplaceComponent, vistle);
    p_operation = addIntParameter("operation", "displacement operation to apply to selected component or element-wise",
                                  DisplaceOperation::Add, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(p_operation, DisplaceOperation, vistle);
    p_scale = addFloatParameter("scale", "scaling factor for displacement", 1.);
}

DisplaceVtkm::~DisplaceVtkm()
{}

std::unique_ptr<viskores::filter::Filter> DisplaceVtkm::setUpFilter() const
{
    auto filter = std::make_unique<DisplaceFilter>();
    filter->SetDisplacementComponent(static_cast<DisplaceComponent>(p_component->getValue()));
    filter->SetDisplacementOperation(static_cast<DisplaceOperation>(p_operation->getValue()));
    filter->SetScale(static_cast<viskores::FloatDefault>(p_scale->getValue()));

    return filter;
}
