#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEFILTER_H

#include <viskores/cont/ErrorBadValue.h>
#include <viskores/filter/Filter.h>

#include <vistle/util/enum.h>

/*
    This Viskores filter displaces the coordinates of a dataset according to a given field.
    Different displacement operations are implemented (Set, Add, Multiply) and can be selected
    with `SetDisplacementOperation`. An additional scaling factor can be set with `SetScale`.
    If the data field is scalar, the displacement can be applied to a specific component 
    (X, Y, Z) or all components, which can be set with `SetDisplacementComponent`.
*/
class DisplaceFilter: public viskores::filter::Filter {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceComponent, (X)(Y)(Z)(All))
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceOperation, (Set)(Add)(Multiply))

    VISKORES_CONT DisplaceFilter();

    VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &dataset) override;

    /*
        Set the component to displace. This is ignored if the data field is not scalar.

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

    /*
        Creates a suitable mask using `m_component`, which needs to be passed to the displace worklets.
    */
    template<int CoordsDim>
    viskores::Vec<viskores::FloatDefault, CoordsDim> createMask()
    {
        viskores::Vec<viskores::FloatDefault, CoordsDim> mask(0.0f);

        viskores::IdComponent c = 0; // must be declared outside switch-case
        switch (m_component) {
        case DisplaceComponent::X:
        case DisplaceComponent::Y:
        case DisplaceComponent::Z:
            c = static_cast<viskores::IdComponent>(m_component);
            if (c < CoordsDim) {
                mask[c] = 1.0f;
            } else {
                throw viskores::cont::ErrorBadValue("Error in DisplaceFilter: DisplaceComponent value (" +
                                                    std::to_string(c) + ") out of bounds for coordinate dimension (" +
                                                    std::to_string(CoordsDim) + ")!");
            }
            break;
        case DisplaceComponent::All:
            for (auto c = 0; c < CoordsDim; c++) {
                mask[c] = 1.0f;
            }
            break;
        default:
            throw viskores::cont::ErrorBadValue(
                "Error in DisplaceFilter: Encountered unknown DisplaceComponent value!");
        }

        return mask;
    }

    /*
        Calls the desired displace operation, defined by `m_operation`, using the provided data field `field`
        and the input coordinates `inputCoords`. The result is stored in `resultCoords`.

        Note: CoordsArrayType and ResultArrayType might have different storage types, so we cannot use one
              type for both the input and output coordinates.
    */
    template<int CoordsDim, typename FieldArrayType, typename CoordsArrayType, typename ResultArrayType>
    void applyDisplaceOperation(const FieldArrayType &field, const CoordsArrayType &inputCoords,
                                ResultArrayType &resultCoords);
};

#endif
