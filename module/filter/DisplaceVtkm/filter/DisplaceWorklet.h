#ifndef VISTLE_DISPLACEVTKM_FILTER_DISPLACEWORKLET_H
#define VISTLE_DISPLACEVTKM_FILTER_DISPLACEWORKLET_H

#include <viskores/worklet/WorkletMapField.h>

#include <vistle/util/enum.h>

template<viskores::IdComponent N>
struct BaseDisplaceWorklet {
    viskores::FloatDefault m_scale;
    viskores::Vec<viskores::FloatDefault, N> m_mask;

    VISKORES_CONT BaseDisplaceWorklet(): m_scale{1.0f}, m_mask{1.0f, 1.0f, 1.0f} {};

    VISKORES_CONT BaseDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, N> mask)
    : m_scale{scale}, m_mask{mask}
    {}
};

template<viskores::IdComponent N>
struct SetDisplaceWorklet: BaseDisplaceWorklet<N>, viskores::worklet::WorkletMapField {
    VISKORES_CONT SetDisplaceWorklet(): BaseDisplaceWorklet<N>(){};

    VISKORES_CONT SetDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, N> mask)
    : BaseDisplaceWorklet<N>(scale, mask)
    {}

    using ControlSignature = void(FieldIn field, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &fieldValue, const viskores::Vec<S, N> &coord,
                                  viskores::Vec<S, N> &displacedCoord) const
    {
        for (auto c = 0; c < N; c++)
            displacedCoord[c] = this->m_mask[c] * (fieldValue * this->m_scale) + (1 - this->m_mask[c]) * coord[c];
    }

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const viskores::Vec<T, N> &fieldValue, const viskores::Vec<S, N> &coord,
                                  viskores::Vec<S, N> &displacedCoord) const
    {
        for (auto c = 0; c < N; c++)
            displacedCoord[c] = fieldValue[c] * this->m_scale;
    }
};

template<viskores::IdComponent N>
struct AddDisplaceWorklet: BaseDisplaceWorklet<N>, viskores::worklet::WorkletMapField {
    VISKORES_CONT AddDisplaceWorklet(): BaseDisplaceWorklet<N>(){};

    VISKORES_CONT AddDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, N> mask)
    : BaseDisplaceWorklet<N>(scale, mask)
    {}

    using ControlSignature = void(FieldIn field, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &fieldValue, const viskores::Vec<S, N> &coord,
                                  viskores::Vec<S, N> &displacedCoord) const
    {
        for (auto c = 0; c < N; c++)
            displacedCoord[c] =
                this->m_mask[c] * (coord[c] + fieldValue * this->m_scale) + (1 - this->m_mask[c]) * coord[c];
    }

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const viskores::Vec<T, N> &fieldValue, const viskores::Vec<S, N> &coord,
                                  viskores::Vec<S, N> &displacedCoord) const
    {
        for (auto c = 0; c < N; c++)
            displacedCoord[c] = coord[c] + fieldValue[c] * this->m_scale;
    }
};

template<viskores::IdComponent N>
struct MultiplyDisplaceWorklet: BaseDisplaceWorklet<N>, viskores::worklet::WorkletMapField {
    VISKORES_CONT MultiplyDisplaceWorklet(): BaseDisplaceWorklet<N>(){};

    VISKORES_CONT MultiplyDisplaceWorklet(viskores::FloatDefault scale, viskores::Vec<viskores::FloatDefault, N> mask)
    : BaseDisplaceWorklet<N>(scale, mask)
    {}

    using ControlSignature = void(FieldIn field, FieldIn coords, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const T &fieldValue, const viskores::Vec<S, N> &coord,
                                  viskores::Vec<S, N> &displacedCoord) const
    {
        for (auto c = 0; c < N; c++)
            displacedCoord[c] =
                this->m_mask[c] * (coord[c] * fieldValue * this->m_scale) + (1 - this->m_mask[c]) * coord[c];
    }

    template<typename T, typename S>
    VISKORES_EXEC void operator()(const viskores::Vec<T, N> &fieldValue, const viskores::Vec<S, N> &coord,
                                  viskores::Vec<S, N> &displacedCoord) const
    {
        for (auto c = 0; c < N; c++)
            displacedCoord[c] = coord[c] * fieldValue[c] * this->m_scale;
    }
};

#endif
