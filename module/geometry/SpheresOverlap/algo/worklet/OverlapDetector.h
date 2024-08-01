#ifndef OVERLAP_DETECTOR_H
#define OVERLAP_DETECTOR_H

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>

#include "../ThicknessDeterminer.h"

/*
    Counts overlaps between spheres and creates connection lines between overlapping spheres.
*/
class OverlapDetector {
public:
    using CoordPortalType = typename vtkm::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;
    using IdPortalType = typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType;
    using FloatPortalType = typename vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType;


    OverlapDetector(const vtkm::Vec3f &min, const vtkm::Vec3f &max, const vtkm::Id3 &nrBins,
                    const CoordPortalType &coords, const FloatPortalType &radii, const IdPortalType &pointIds,
                    const IdPortalType &cellLowerBounds, const IdPortalType &cellUpperBounds,
                    ThicknessDeterminer determiner, const vtkm::cont::ArrayHandle<vtkm::Int8>::ReadPortalType &offsets)
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

    VTKM_EXEC void CountOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &point, vtkm::Id &nrOverlaps) const;
    VTKM_EXEC void CreateConnectionLines(const vtkm::Id pointId, const vtkm::Vec3f &point,
                                         const vtkm::IdComponent visitId, vtkm::Id2 &connectivity,
                                         vtkm::FloatDefault &thickness) const;

    VTKM_EXEC vtkm::Id3 DetermineCellId(const vtkm::Vec3f &point) const;
    VTKM_EXEC vtkm::Id FlattenCellId(const vtkm::Id3 &cellId) const;
    VTKM_EXEC bool CellExists(const vtkm::Id3 &cellId) const;

private:
    vtkm::Vec3f Min;
    vtkm::Id3 Dims;
    vtkm::Vec3f Dxdydz;

    CoordPortalType Coords;
    FloatPortalType Radii;

    IdPortalType PointIds;
    IdPortalType LowerBounds;
    IdPortalType UpperBounds;

    ThicknessDeterminer Determiner = OverlapRatio;

    vtkm::cont::ArrayHandle<vtkm::Int8>::ReadPortalType OffsetsToNeighbors;
};

#endif // OVERLAP_DETECTOR_H
