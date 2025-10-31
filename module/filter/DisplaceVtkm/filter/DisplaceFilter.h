#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H

#include <viskores/filter/Filter.h>

#include <vistle/util/enum.h>

/*
    This Viskores filter displaces the coordinates of a dataset according to a scalar field.
    The displacement can be applied to a specific component (X, Y, Z) or all components (see
    `SetDisplacementComponent`) and supports different operations (Set, Add, Multiply; see
    `SetDisplacementOperation`). An additional scaling factor can be set with `SetScale`.
*/
class DisplaceFilter: public viskores::filter::Filter {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceComponent, (X)(Y)(Z)(All))
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceOperation, (Set)(Add)(Multiply))

    VISKORES_CONT DisplaceFilter();

    VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &dataset) override;

    /*
        Set the component to displace.

        Available values:
            `DisplaceComponent::X`
            `DisplaceComponent::Y`
            `DisplaceComponent::Z`
            `DisplaceComponent::All`
    */
    VISKORES_CONT void SetDisplacementComponent(const DisplaceComponent &component) { m_component = component; }
    /*
        Set the operation to apply for displacement.

        Available values:
            `DisplaceOperation::Set` - sets the selected component(s) to the value in the scalar field
            `DisplaceOperation::Add` - adds the value in the scalar field to the selected component(s)
            `DisplaceOperation::Multiply` - multiplies the selected component(s) by the value in the scalar field
    */
    VISKORES_CONT void SetDisplacementOperation(const DisplaceOperation &operation) { m_operation = operation; }

    /*
        Set the scaling factor for displacement.
    */
    VISKORES_CONT void SetScale(const viskores::FloatDefault &scale) { m_scale = scale; }

    VISKORES_CONT DisplaceComponent GetDisplacementComponent() const { return m_component; }
    VISKORES_CONT DisplaceOperation GetDisplacementOperation() const { return m_operation; }
    VISKORES_CONT viskores::FloatDefault GetScale() const { return m_scale; }

private:
    DisplaceComponent m_component;
    DisplaceOperation m_operation;
    viskores::FloatDefault m_scale;
};

#endif
