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

        VecType p0;
        VecType p1;
        switch (Attachment) {
        case 0: // Bottom
            p0 = base;
            p1 = base + v;
            break;
        case 1: // Middle
            p0 = base - v * ScalarType(0.5);
            p1 = base + v * ScalarType(0.5);
            break;
        case 2: // Top
            p0 = base - v;
            p1 = base;
            break;
        default:
            p0 = base;
            p1 = base + v;
            break;
        }

        using OutVec = typename PortalOut::ValueType;
        using OutScalar = typename OutVec::ComponentType;

        const viskores::Id out0 = 2 * idx;
        coordsPortal.Set(out0, OutVec(static_cast<OutScalar>(p0[0]), static_cast<OutScalar>(p0[1]),
                                      static_cast<OutScalar>(p0[2])));
        coordsPortal.Set(out0 + 1, OutVec(static_cast<OutScalar>(p1[0]), static_cast<OutScalar>(p1[1]),
                                          static_cast<OutScalar>(p1[2])));
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
