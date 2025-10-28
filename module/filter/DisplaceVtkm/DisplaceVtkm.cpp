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
    auto filter = std::make_unique<viskores::filter::field_transform::Warp>();

    // TODO: implement Set and Multiply operations!
    if (p_operation->getValue() == DisplaceOperation::Add) {
        switch (p_component->getValue()) {
        case DisplaceComponent::X:
            filter->SetConstantDirection({1.0f, 0.0f, 0.0f});
            break;
        case DisplaceComponent::Y:
            filter->SetConstantDirection({0.0f, 1.0f, 0.0f});
            break;
        case DisplaceComponent::Z:
            filter->SetConstantDirection({0.0f, 0.0f, 1.0f});
            break;
        case DisplaceComponent::All:
            filter->SetConstantDirection({1.0f, 1.0f, 1.0f});
            break;
        }
        filter->SetUseConstantDirection(true);
        filter->SetScaleField(getFieldName(0, false));
        filter->SetScaleFactor(p_scale->getValue());
    }
    return filter;
}

DataBase::ptr DisplaceVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                               const vistle::Object::const_ptr &inputGrid,
                                               const vistle::DataBase::const_ptr &inputField,
                                               const std::string &fieldName,
                                               const vistle::Object::const_ptr &outputGrid) const
{
    // The default output field of the Warp filter seems to contain the adjusted point coordinates.
    // To match Displace's behavior, we need to pass the original scalar field (which is also part
    // of the output dataset, luckily).
    return VtkmModule::prepareOutputField(dataset, inputGrid, inputField,
                                          fieldName == getFieldName(0, true) ? getFieldName(0, false) : fieldName,
                                          outputGrid);
}
