#include "GenerateSeeds.h"

namespace vistle {
VISKORES_EXEC void GenerateSeedsOnPlaneWorklet::operator()(viskores::Particle &seed, const viskores::Id &index) const
{
    viskores::Id i = index / m_n1;
    viskores::Id j = index % m_n1;

    viskores::FloatDefault s0 = viskores::FloatDefault(1) / (m_n0 - 1);
    viskores::FloatDefault s1 = viskores::FloatDefault(1) / (m_n1 - 1);
    auto point = m_startpoint1 + m_parallelSpan * s0 * i + m_orthogonalSpan * s1 * j;
    seed = viskores::Particle({point[0], point[1], point[2]}, index);
}

VISKORES_EXEC void GenerateSeedsOnLineWorklet::operator()(viskores::Particle &seed, const viskores::Id &index) const
{
    seed = viskores::Particle({m_startpoint1[0] + index * m_delta[0], m_startpoint1[1] + index * m_delta[1],
                               m_startpoint1[2] + index * m_delta[2]},
                              index);
}

namespace {

void computePlaneSpans(const viskores::Vec3f &startpoint1, const viskores::Vec3f &startpoint2,
                       const viskores::Vec3f &direction, viskores::Vec3f &parallelSpan, viskores::Vec3f &orthogonalSpan)
{
    auto normedDirection = direction;
    viskores::Normalize(normedDirection);
    auto seedSpan = startpoint2 - startpoint1;
    parallelSpan = normedDirection * viskores::Dot(normedDirection, seedSpan);
    orthogonalSpan = seedSpan - parallelSpan;
}

void computeSeedCountPerSpan(viskores::Id numSeeds, viskores::FloatDefault length0, viskores::FloatDefault length1,
                             viskores::Id &count0, viskores::Id &count1)
{
    if (length0 > length1) {
        count1 = viskores::Id(viskores::Sqrt(numSeeds * length1 / length0)) + 1;
        if (count1 <= 1)
            count1 = 2;
        count0 = numSeeds / count1;
        if (count0 <= 1)
            count0 = 2;
    } else {
        count0 = viskores::Id(viskores::Sqrt(numSeeds * length0 / length1)) + 1;
        if (count0 <= 1)
            count0 = 2;
        count1 = numSeeds / count0;
        if (count1 <= 1)
            count1 = 2;
    }
}
} // namespace

viskores::cont::ArrayHandle<viskores::Particle> generateSeedsOnPlane(viskores::Id numSeeds,
                                                                     const viskores::Vec3f &startpoint1,
                                                                     const viskores::Vec3f &startpoint2,
                                                                     const viskores::Vec3f &direction)
{
    viskores::Vec3f parallelSpan, orthogonalSpan;
    computePlaneSpans(startpoint1, startpoint2, direction, parallelSpan, orthogonalSpan);

    viskores::Id n0, n1;
    computeSeedCountPerSpan(numSeeds, viskores::Magnitude(parallelSpan), viskores::Magnitude(orthogonalSpan), n0, n1);

    numSeeds = n0 * n1;
    viskores::cont::ArrayHandle<viskores::Particle> seedArray;
    seedArray.Allocate(numSeeds);

    viskores::worklet::DispatcherMapField<GenerateSeedsOnPlaneWorklet>(
        GenerateSeedsOnPlaneWorklet(numSeeds, n0, n1, startpoint1, startpoint2, parallelSpan, orthogonalSpan))
        .Invoke(seedArray);

    return seedArray;
}

viskores::cont::ArrayHandle<viskores::Particle>
generateSeedsOnLine(viskores::Id numSeeds, const viskores::Vec3f &startpoint1, const viskores::Vec3f &startpoint2)
{
    viskores::cont::ArrayHandle<viskores::Particle> seedArray;
    seedArray.Allocate(numSeeds);

    if (numSeeds == 1) {
        auto point = (startpoint1 + startpoint2) * 0.5;
        seedArray.WritePortal().Set(0, viskores::Particle({point[0], point[1], point[2]}, 0));
    } else {
        viskores::worklet::DispatcherMapField<GenerateSeedsOnLineWorklet>(
            GenerateSeedsOnLineWorklet(numSeeds, startpoint1, startpoint2))
            .Invoke(seedArray);
    }

    return seedArray;
}

} // namespace vistle
