#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEWORKLET_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEWORKLET_H

#include <viskores/worklet/WorkletMapField.h>

#include <vistle/util/enum.h>

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceComponent, (X)(Y)(Z)(All))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(DisplaceOperation, (Set)(Add)(Multiply))

} // namespace vistle

struct BaseDisplaceWorklet {
    viskores::FloatDefault m_scale;
    viskores::Vec<viskores::FloatDefault, 3> m_mask;

    VISKORES_CONT BaseDisplaceWorklet(): m_scale{1.0f}, m_mask{1.0f, 1.0f, 1.0f} {};

    VISKORES_CONT BaseDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, 3> mask)
    : m_scale{scale}, m_mask{mask}
    {}
};

struct SetDisplaceWorklet: BaseDisplaceWorklet, viskores::worklet::WorkletMapField {
    VISKORES_CONT SetDisplaceWorklet(): BaseDisplaceWorklet(){};

    VISKORES_CONT SetDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, 3> mask)
    : BaseDisplaceWorklet(scale, mask)
    {}

    using ControlSignature = void(FieldIn scalarField, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &scalar, const viskores::Vec<S, 3> &coord,
                                  viskores::Vec<S, 3> &displacedCoord) const
    {
        for (auto c = 0; c < 3; c++)
            displacedCoord[c] = m_mask[c] * (scalar * m_scale) + (1 - m_mask[c]) * coord[c];
    }
};

struct AddDisplaceWorklet: BaseDisplaceWorklet, viskores::worklet::WorkletMapField {
    VISKORES_CONT AddDisplaceWorklet(): BaseDisplaceWorklet(){};

    VISKORES_CONT AddDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, 3> mask)
    : BaseDisplaceWorklet(scale, mask)
    {}

    using ControlSignature = void(FieldIn scalarField, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &scalar, const viskores::Vec<S, 3> &coord,
                                  viskores::Vec<S, 3> &displacedCoord) const
    {
        for (auto c = 0; c < 3; c++)
            displacedCoord[c] = m_mask[c] * (coord[c] + scalar * m_scale) + (1 - m_mask[c]) * coord[c];
    }
};

struct MultiplyDisplaceWorklet: BaseDisplaceWorklet, viskores::worklet::WorkletMapField {
    VISKORES_CONT MultiplyDisplaceWorklet(): BaseDisplaceWorklet(){};

    VISKORES_CONT MultiplyDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, 3> mask)
    : BaseDisplaceWorklet(scale, mask)
    {}

    using ControlSignature = void(FieldIn scalarField, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &scalar, const viskores::Vec<S, 3> &coord,
                                  viskores::Vec<S, 3> &displacedCoord) const
    {
        for (auto c = 0; c < 3; c++)
            displacedCoord[c] = m_mask[c] * (coord[c] * scalar * m_scale) + (1 - m_mask[c]) * coord[c];
    }
};

#endif
