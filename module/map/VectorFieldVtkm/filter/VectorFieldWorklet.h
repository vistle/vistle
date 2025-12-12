#ifndef VISTLE_VECTORFIELDVTKM_FILTER_VECTORFIELDWORKLET_H
#define VISTLE_VECTORFIELDVTKM_FILTER_VECTORFIELDWORKLET_H

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/Math.h>

#include <vistle/core/scalar.h>

namespace viskores {
namespace worklet {

struct VectorFieldWorklet: public viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn vec, // input vectors
                                  FieldIn basePos, // input base positions (points or cell centers)
                                  WholeArrayOut coords // output line endpoints interleaved: [p0, p1] per input
    );

    using ExecutionSignature = void(_1, _2, _3, WorkIndex);
    using InputDomain = _1;

    VISKORES_CONT
    VectorFieldWorklet(viskores::FloatDefault minLen, viskores::FloatDefault maxLen, viskores::FloatDefault scale,
                       viskores::IdComponent attachmentPoint)
    : MinLength(minLen), MaxLength(maxLen), Scale(scale), Attachment(attachmentPoint)
    {}

    template<typename VecType, typename PortalOut>
    VISKORES_EXEC void operator()(const VecType &vec, const VecType &base, const PortalOut &coordsPortal,
                                  viskores::Id idx) const
    {
        VecType v = vec;

        const auto length = viskores::Magnitude(v);
        using ScalarType = decltype(length);
        const ScalarType zero(0);
        const ScalarType one(1);

        const ScalarType minLen = static_cast<ScalarType>(MinLength);
        const ScalarType maxLen = static_cast<ScalarType>(MaxLength);
        const ScalarType scale = static_cast<ScalarType>(Scale);

        // Clamp length and avoid divide-by-zero without branching.
        const ScalarType clampedLength = viskores::Max(minLen, viskores::Min(length, maxLen));
        const ScalarType hasLength = static_cast<ScalarType>(length > zero);
        const ScalarType safeLength = hasLength * length + (one - hasLength);
        const ScalarType scaleFactor = scale * (hasLength * clampedLength) / safeLength;

        v = v * scaleFactor;

        const ScalarType attachment =
            viskores::Max(ScalarType(0), viskores::Min(static_cast<ScalarType>(Attachment), ScalarType(2)));
        // attachment: 0=Bottom, 1=Middle, 2=Top -> offset: 0, 0.5, 1
        const ScalarType offset = attachment * ScalarType(0.5);

        const VecType p0 = base - v * offset;
        const VecType p1 = base + v * (one - offset);

        using OutVec = typename PortalOut::ValueType;
        using OutScalar = typename OutVec::ComponentType;

        const viskores::Id out0 = 2 * idx;
        coordsPortal.Set(
            out0, OutVec(static_cast<OutScalar>(p0[0]), static_cast<OutScalar>(p0[1]), static_cast<OutScalar>(p0[2])));
        coordsPortal.Set(out0 + 1, OutVec(static_cast<OutScalar>(p1[0]), static_cast<OutScalar>(p1[1]),
                                          static_cast<OutScalar>(p1[2])));
    }

private:
    viskores::FloatDefault MinLength;
    viskores::FloatDefault MaxLength;
    viskores::FloatDefault Scale;
    viskores::IdComponent Attachment; // 0=Bottom, 1=Middle, 2=Top (match Vistle enum values)
};

struct DuplicateFieldToEndpoints: public viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn value, WholeArrayOut expanded);
    using ExecutionSignature = void(_1, _2, WorkIndex);
    using InputDomain = _1;

    template<typename T, typename PortalOut>
    VISKORES_EXEC void operator()(const T &val, const PortalOut &portal, viskores::Id idx) const
    {
        const viskores::Id out0 = 2 * idx;
        portal.Set(out0, val);
        portal.Set(out0 + 1, val);
    }
};

} // namespace worklet
} // namespace viskores

#endif
