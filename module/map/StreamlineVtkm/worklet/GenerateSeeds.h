#ifndef VISTLE_STREAMLINEVTKM_WORKLET_GENERATESEEDS_H
#define VISTLE_STREAMLINEVTKM_WORKLET_GENERATESEEDS_H

#include <viskores/Particle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace vistle {

class GenerateSeedsOnPlaneWorklet: public viskores::worklet::WorkletMapField {
public:
    VISKORES_CONT
    GenerateSeedsOnPlaneWorklet(viskores::Id numSeeds, viskores::Id n0, viskores::Id n1,
                                const viskores::Vec3f &startpoint1, const viskores::Vec3f &startpoint2,
                                const viskores::Vec3f &parallelSpan, const viskores::Vec3f &orthogonalSpan)
    : m_numSeeds(numSeeds)
    , m_n0(n0)
    , m_n1(n1)
    , m_startpoint1(startpoint1)
    , m_startpoint2(startpoint2)
    , m_parallelSpan(parallelSpan)
    , m_orthogonalSpan(orthogonalSpan)
    {}

    using ControlSignature = void(FieldOut seeds);
    using ExecutionSignature = void(_1, WorkIndex);

    VISKORES_EXEC void operator()(viskores::Particle &seed, const viskores::Id &index) const;

private:
    viskores::Id m_numSeeds;

    viskores::Id m_n0;
    viskores::Id m_n1;

    viskores::Vec3f m_startpoint1;
    viskores::Vec3f m_startpoint2;
    viskores::Vec3f m_parallelSpan;
    viskores::Vec3f m_orthogonalSpan;
};

class GenerateSeedsOnLineWorklet: public viskores::worklet::WorkletMapField {
public:
    VISKORES_CONT GenerateSeedsOnLineWorklet(viskores::Id numSeeds, const viskores::Vec3f &startpoint1,
                                             const viskores::Vec3f &startpoint2)
    : m_numSeeds(numSeeds)
    , m_startpoint1(startpoint1)
    , m_startpoint2(startpoint2)
    , m_delta((m_startpoint2 - m_startpoint1) / (m_numSeeds - 1))
    {}

    VISKORES_CONT GenerateSeedsOnLineWorklet(viskores::Id numSeeds, const viskores::Vec3f &startpoint1,
                                             const viskores::Vec3f &startpoint2, const viskores::Vec3f &delta)
    : m_numSeeds(numSeeds), m_startpoint1(startpoint1), m_startpoint2(startpoint2), m_delta(delta)
    {}

    using ControlSignature = void(FieldOut seeds);
    using ExecutionSignature = void(_1, WorkIndex);

    VISKORES_EXEC void operator()(viskores::Particle &seed, const viskores::Id &index) const;

private:
    viskores::Id m_numSeeds;

    viskores::Vec3f m_startpoint1;
    viskores::Vec3f m_startpoint2;
    viskores::Vec3f m_delta;
};

viskores::cont::ArrayHandle<viskores::Particle> generateSeedsOnPlane(viskores::Id numSeeds,
                                                                     const viskores::Vec3f &startpoint1,
                                                                     const viskores::Vec3f &startpoint2,
                                                                     const viskores::Vec3f &direction);

viskores::cont::ArrayHandle<viskores::Particle>
generateSeedsOnLine(viskores::Id numSeeds, const viskores::Vec3f &startpoint1, const viskores::Vec3f &startpoint2);

}; // namespace vistle

#endif // VISTLE_STREAMLINEVTKM_WORKLET_GENERATESEEDS_H
