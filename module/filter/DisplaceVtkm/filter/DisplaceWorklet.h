#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEWORKLET_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEWORKLET_H

#include <viskores/worklet/WorkletMapField.h>

#include <vistle/util/enum.h>

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceComponent, (X)(Y)(Z)(All))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceOperation, (Set)(Add)(Multiply))

template<typename T, typename S>
VISKORES_EXEC T applyOperation(const T &value, const S &displacement, DisplaceOperation operation)
{
    switch (operation) {
    case Set:
        return displacement;
    case Add:
        return value + displacement;
    case Multiply:
        return value * displacement;
    default:
        std::cerr << "DisplaceVtkmWorklet: Unknown DisplaceOperation " << static_cast<int>(operation) << std::endl;
        return value;
    }
}

struct DisplaceVtkmWorklet: viskores::worklet::WorkletMapField {
    DisplaceComponent m_component;
    DisplaceOperation m_operation;
    viskores::FloatDefault m_scale;

    VISKORES_CONT DisplaceVtkmWorklet()
    : m_component(DisplaceComponent::X), m_operation(DisplaceOperation::Add), m_scale(1.0f)
    {}

    VISKORES_CONT DisplaceVtkmWorklet(DisplaceComponent component, DisplaceOperation operation,
                                      viskores::FloatDefault scale)
    : m_component(component), m_operation(operation), m_scale(scale)
    {}

    using ControlSignature = void(FieldIn scalarField, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &scalar, const viskores::Vec<S, 3> &coord,
                                  viskores::Vec<S, 3> &displacedCoord) const
    {
        displacedCoord = coord;
        switch (m_component) {
        case X:
            displacedCoord[0] = applyOperation(coord[0], scalar * m_scale, m_operation);
            break;
        case Y:
            displacedCoord[1] = applyOperation(coord[1], scalar * m_scale, m_operation);
            break;
        case Z:
            displacedCoord[2] = applyOperation(coord[2], scalar * m_scale, m_operation);
            break;
        case All:
            for (int c = 0; c < 3; ++c) {
                displacedCoord[c] = applyOperation(coord[c], scalar * m_scale, m_operation);
            }
            break;
        }
    }
};

} // namespace vistle

#endif
