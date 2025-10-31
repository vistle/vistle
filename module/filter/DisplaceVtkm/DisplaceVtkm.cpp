#include "filter/DisplaceFilter.h"

#include "DisplaceVtkm.h"

MODULE_MAIN(DisplaceVtkm)

using namespace vistle;

DisplaceVtkm::DisplaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 5, MappedDataHandling::Require)
{
    p_component = addIntParameter("component", "component to displace for scalar input",
                                  DisplaceFilter::DisplaceComponent::Z, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(p_component, DisplaceComponent, DisplaceFilter);

    p_operation = addIntParameter("operation", "displacement operation to apply to selected component or element-wise",
                                  DisplaceFilter::DisplaceOperation::Add, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(p_operation, DisplaceOperation, DisplaceFilter);

    p_scale = addFloatParameter("scale", "scaling factor for displacement", 1.);
}

DisplaceVtkm::~DisplaceVtkm()
{}

std::unique_ptr<viskores::filter::Filter> DisplaceVtkm::setUpFilter() const
{
    auto filter = std::make_unique<DisplaceFilter>();

    filter->SetDisplacementComponent(static_cast<DisplaceFilter::DisplaceComponent>(p_component->getValue()));
    filter->SetDisplacementOperation(static_cast<DisplaceFilter::DisplaceOperation>(p_operation->getValue()));
    filter->SetScale(static_cast<viskores::FloatDefault>(p_scale->getValue()));

    return filter;
}
