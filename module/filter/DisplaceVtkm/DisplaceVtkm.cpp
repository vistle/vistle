#include <viskores/filter/field_transform/Warp.h>

#include "DisplaceVtkm.h"

MODULE_MAIN(DisplaceVtkm)

using namespace vistle;

DisplaceVtkm::DisplaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Require)
{
    p_component =
        addIntParameter("component", "component to displace for scalar input", DisplaceComponent::Z, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_component, DisplaceComponent);
    p_operation = addIntParameter("operation", "displacement operation to apply to selected component or element-wise",
                                  DisplaceOperation::Add, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_operation, DisplaceOperation);

    p_scale = addFloatParameter("scale", "scaling factor for displacement", 1.);
}

DisplaceVtkm::~DisplaceVtkm()
{}

std::unique_ptr<viskores::filter::Filter> DisplaceVtkm::setUpFilter() const
{
    return std::make_unique<viskores::filter::field_transform::Warp>();
}
