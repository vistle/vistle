#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H

#include <viskores/filter/Filter.h>

#include "DisplaceWorklet.h"

/*
    This Viskores filter displaces the coordinates of a dataset according to a scalar field.
    The displacement can be applied to a specific component (X, Y, Z) or all components (see
    `SetDisplacementComponent`) and supports different operations (Set, Add, Multiply; see
    `SetDisplacementOperation`). An additional scaling factor can be set with `SetScale`.
*/
class DisplaceFilter: public viskores::filter::Filter {
private:
    vistle::DisplaceComponent m_component;
    vistle::DisplaceOperation m_operation;
    viskores::FloatDefault m_scale;

public:
    VISKORES_CONT DisplaceFilter();

    VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &dataset) override;

    /*
        Set the component to displace.

        Available values:
            `vistle::DisplaceComponent::X`
            `vistle::DisplaceComponent::Y`
            `vistle::DisplaceComponent::Z`
            `vistle::DisplaceComponent::All`
    */
    VISKORES_CONT void SetDisplacementComponent(const vistle::DisplaceComponent &component) { m_component = component; }
    /*
        Set the operation to apply for displacement.

        Available values:
            `vistle::DisplaceOperation::Set` - sets the selected component(s) to the value in the scalar field
            `vistle::DisplaceOperation::Add` - adds the value in the scalar field to the selected component(s)
            `vistle::DisplaceOperation::Multiply` - multiplies the selected component(s) by the value in the scalar field
    */
    VISKORES_CONT void SetDisplacementOperation(const vistle::DisplaceOperation &operation) { m_operation = operation; }

    /*
        Set the scaling factor for displacement.
    */
    VISKORES_CONT void SetScale(const viskores::FloatDefault &scale) { m_scale = scale; }

    VISKORES_CONT vistle::DisplaceComponent GetDisplacementComponent() const { return m_component; }
    VISKORES_CONT vistle::DisplaceOperation GetDisplacementOperation() const { return m_operation; }
    VISKORES_CONT viskores::FloatDefault GetScale() const { return m_scale; }
};

#endif
