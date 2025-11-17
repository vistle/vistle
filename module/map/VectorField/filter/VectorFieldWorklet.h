#ifndef VISTLE_VECTORFIELD_FILTER_VECTORFIELDWORKLET_H
#define VISTLE_VECTORFIELD_FILTER_VECTORFIELDWORKLET_H

#include <viskores/Types.h>
#include <viskores/worklet/WorkletMapField.h>

struct VectorFieldWorklet : public viskores::worklet::WorkletMapField {
    using Vec3 = viskores::Vec<viskores::FloatDefault, 3>;

    // Input: one vector + one position per work item
    // Output: two endpoints per work item (p0, p1), stored in a single array
    using ControlSignature = void(FieldIn vecField, FieldIn coords, WholeArrayOut endpoints);
    using ExecutionSignature = void(_1, _2, _3, WorkIndex);
    using InputDomain = _1;

    // Parameters set from host (VectorField.cpp)
    viskores::FloatDefault MinLength = 0.0f;
    viskores::FloatDefault MaxLength = 0.0f;
    viskores::FloatDefault Scale     = 1.0f;
    // 0 = Bottom, 1 = Middle, 2 = Top (matches AttachmentPoint enum)
    viskores::IdComponent Attachment = 0;

    template <typename VecInType, typename CoordInType, typename PortalOutType>
    VISKORES_EXEC void operator()(const VecInType &vecIn,
                                  const CoordInType &coordIn,
                                  PortalOutType &endpointPortal,
                                  const viskores::Id &workIdx) const
    {
        Vec3 v(static_cast<viskores::FloatDefault>(vecIn[0]),
               static_cast<viskores::FloatDefault>(vecIn[1]),
               static_cast<viskores::FloatDefault>(vecIn[2]));

        Vec3 p(static_cast<viskores::FloatDefault>(coordIn[0]),
               static_cast<viskores::FloatDefault>(coordIn[1]),
               static_cast<viskores::FloatDefault>(coordIn[2]));

        // Clamp vector length
        viskores::FloatDefault len = viskores::Magnitude(v);
        if (len > viskores::FloatDefault(0)) {
            if (len < MinLength) {
                v = v * (MinLength / len);
            } else if (len > MaxLength) {
                v = v * (MaxLength / len);
            }
        }

        // Scale
        v = v * Scale;

        // Attachment: Bottom/Middle/Top
        Vec3 p0, p1;
        switch (Attachment) {
        case 0: // Bottom
            p0 = p;
            p1 = p + v;
            break;
        case 1: // Middle
            p0 = p - static_cast<viskores::FloatDefault>(0.5f) * v;
            p1 = p + static_cast<viskores::FloatDefault>(0.5f) * v;
            break;
        case 2: // Top
        default:
            p0 = p - v;
            p1 = p;
            break;
        }

        // Each work index writes two endpoints into a flat array
        const viskores::Id base = static_cast<viskores::Id>(2) * workIdx;
        endpointPortal.Set(base,     p0);
        endpointPortal.Set(base + 1, p1);
    }
};

#endif // VISTLE_VECTORFIELD_FILTER_VECTORFIELDWORKLET_H
