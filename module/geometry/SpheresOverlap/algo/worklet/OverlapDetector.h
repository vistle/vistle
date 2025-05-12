#ifndef VISTLE_SPHERESOVERLAP_ALGO_WORKLET_OVERLAPDETECTOR_H
#define VISTLE_SPHERESOVERLAP_ALGO_WORKLET_OVERLAPDETECTOR_H

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>

#include "../ThicknessDeterminer.h"

/*
    Counts overlaps between spheres and creates connection lines between overlapping spheres.
*/
class OverlapDetector {
public:
    using CoordPortalType = typename viskores::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;
    using IdPortalType = typename viskores::cont::ArrayHandle<viskores::Id>::ReadPortalType;
    using FloatPortalType = typename viskores::cont::ArrayHandle<viskores::FloatDefault>::ReadPortalType;


    OverlapDetector(const viskores::Vec3f &min, const viskores::Vec3f &max, const viskores::Id3 &nrBins,
                    const CoordPortalType &coords, const FloatPortalType &radii, const IdPortalType &pointIds,
                    const IdPortalType &cellLowerBounds, const IdPortalType &cellUpperBounds,
                    ThicknessDeterminer determiner,
                    const viskores::cont::ArrayHandle<viskores::Int8>::ReadPortalType &offsets)
    : Min(min)
    , Dims(nrBins)
    , Dxdydz((max - min) / Dims)
    , Coords(coords)
    , Radii(radii)
    , PointIds(pointIds)
    , LowerBounds(cellLowerBounds)
    , UpperBounds(cellUpperBounds)
    , Determiner(determiner)
    , OffsetsToNeighbors(offsets)
    {}

    VISKORES_EXEC void CountOverlaps(const viskores::Id pointId, const viskores::Vec3f &point,
                                     viskores::Id &nrOverlaps) const;
    VISKORES_EXEC void CreateConnectionLines(const viskores::Id pointId, const viskores::Vec3f &point,
                                             const viskores::IdComponent visitId, viskores::Id2 &connectivity,
                                             viskores::FloatDefault &thickness) const;

    VISKORES_EXEC viskores::Id3 DetermineCellId(const viskores::Vec3f &point) const;
    VISKORES_EXEC viskores::Id FlattenCellId(const viskores::Id3 &cellId) const;
    VISKORES_EXEC bool CellExists(const viskores::Id3 &cellId) const;

private:
    viskores::Vec3f Min;
    viskores::Id3 Dims;
    viskores::Vec3f Dxdydz;

    CoordPortalType Coords;
    FloatPortalType Radii;

    IdPortalType PointIds;
    IdPortalType LowerBounds;
    IdPortalType UpperBounds;

    ThicknessDeterminer Determiner = OverlapRatio;

    viskores::cont::ArrayHandle<viskores::Int8>::ReadPortalType OffsetsToNeighbors;
};

#endif
