#ifndef VISTLE_VECTORFIELDVTKM_FILTER_VECTORFIELDWORKLET_H
#define VISTLE_VECTORFIELDVTKM_FILTER_VECTORFIELDWORKLET_H

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/Math.h>

#include <vistle/core/scalar.h>

namespace viskores {
namespace worklet {

struct ComputeCellCenters: public viskores::worklet::WorkletVisitCellsWithPoints {
    using ControlSignature = void(CellSetIn, FieldInPoint coords, FieldOutCell center);
    using ExecutionSignature = void(_2, PointCount, _3);

    template<typename CoordVecType, typename OutType>
    VISKORES_EXEC void operator()(const CoordVecType &coords, viskores::IdComponent numPoints, OutType &center) const
    {
        using ValueType = typename OutType::ComponentType;

        if (numPoints <= 0) {
            center = OutType(ValueType(0));
            return;
        }

        OutType accum = coords[0];
        for (viskores::IdComponent i = 1; i < numPoints; ++i) {
            accum = accum + coords[i];
        }

        const ValueType denom = static_cast<ValueType>(numPoints > 0 ? numPoints : 1);
        center = accum / denom;
    }
};

template<typename CoordType>
struct ExpandCoordsWorklet: public viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn idx, WholeArrayIn p0, WholeArrayIn p1, WholeArrayOut coords);
    using ExecutionSignature = void(_1, _2, _3, _4);
    using InputDomain = _1;

    template<typename PortalIn0, typename PortalIn1, typename PortalOut>
    VISKORES_EXEC void operator()(viskores::Id i, const PortalIn0 &p0Portal, const PortalIn1 &p1Portal,
                                  const PortalOut &coordPortal) const
    {
        const viskores::Id src = i >> 1; // i/2
        const bool useP1 = (i & 1) != 0;
        const auto v = useP1 ? p1Portal.Get(src) : p0Portal.Get(src);
        coordPortal.Set(i, CoordType(static_cast<vistle::Scalar>(v[0]), static_cast<vistle::Scalar>(v[1]),
                                     static_cast<vistle::Scalar>(v[2])));
    }
};

struct VectorFieldWorklet: public viskores::worklet::WorkletMapField {
    using ControlSignature = void(FieldIn vec, // input vectors
                                  FieldIn basePos, // input base positions (points or cell centers)
                                  FieldOut p0, // output start points
                                  FieldOut p1 // output end points
    );

    using ExecutionSignature = void(_1, _2, _3, _4);
    using InputDomain = _1;

    VISKORES_CONT
    VectorFieldWorklet(viskores::FloatDefault minLen, viskores::FloatDefault maxLen, viskores::FloatDefault scale,
                       viskores::IdComponent attachmentPoint)
    : MinLength(minLen), MaxLength(maxLen), Scale(scale), Attachment(attachmentPoint)
    {}

    template<typename VecType>
    VISKORES_EXEC void operator()(const VecType &vec, const VecType &base, VecType &outP0, VecType &outP1) const
    {
        VecType v = vec;

        auto length = viskores::Magnitude(v);
        using ScalarType = decltype(length);
        const ScalarType zero(0);

        const ScalarType minLen = static_cast<ScalarType>(MinLength);
        const ScalarType maxLen = static_cast<ScalarType>(MaxLength);
        const ScalarType scale = static_cast<ScalarType>(Scale);

        if (length > zero) {
            if (length < minLen) {
                const ScalarType factor = minLen / length;
                v = v * factor;
            } else if (length > maxLen) {
                const ScalarType factor = maxLen / length;
                v = v * factor;
            }
        }

        v = v * scale;

        switch (Attachment) {
        case 0: // Bottom
            outP0 = base;
            outP1 = base + v;
            break;
        case 1: // Middle
            outP0 = base - v * ScalarType(0.5);
            outP1 = base + v * ScalarType(0.5);
            break;
        case 2: // Top
            outP0 = base - v;
            outP1 = base;
            break;
        default:
            outP0 = base;
            outP1 = base + v;
            break;
        }
    }

private:
    viskores::FloatDefault MinLength;
    viskores::FloatDefault MaxLength;
    viskores::FloatDefault Scale;
    viskores::IdComponent Attachment; // 0=Bottom, 1=Middle, 2=Top (match Vistle enum values)
};

} // namespace worklet
} // namespace viskores

#endif
