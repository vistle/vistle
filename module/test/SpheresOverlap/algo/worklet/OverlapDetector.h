#ifndef OVERLAP_DETECTOR_H
#define OVERLAP_DETECTOR_H

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>

// TODO: better to avoid duplicate checks (to avoid duplicate lines) or
//       unbalanced thread loads (lower ids have more comparisons than higher ids)?
class OverlapDetector {
public:
    using CoordPortalType = typename vtkm::cont::CoordinateSystem::MultiplexerArrayType::ReadPortalType;
    using IdPortalType = typename vtkm::cont::ArrayHandle<vtkm::Id>::ReadPortalType;
    using FloatPortalType = typename vtkm::cont::ArrayHandle<vtkm::FloatDefault>::ReadPortalType;


    OverlapDetector(const vtkm::Vec3f &min, const vtkm::Vec3f &max, const vtkm::Id3 &nrBins,
                    const CoordPortalType &coords, const FloatPortalType &radii, const IdPortalType &pointIds,
                    const IdPortalType &cellLowerBounds, const IdPortalType &cellUpperBounds)
    : Min(min)
    , Dims(nrBins)
    , Dxdydz((max - Min) / Dims)
    , Coords(coords)
    , Radii(radii)
    , PointIds(pointIds)
    , LowerBounds(cellLowerBounds)
    , UpperBounds(cellUpperBounds)
    {}

    VTKM_EXEC void CountOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &point, vtkm::Id &nrOverlaps) const;
    VTKM_EXEC void CreateOverlapLines(const vtkm::Id pointId, const vtkm::Vec3f &point, const vtkm::IdComponent visitId,
                                      vtkm::Id2 &connectivity, vtkm::FloatDefault &thickness) const;

private:
    vtkm::Vec3f Min;
    vtkm::Id3 Dims;
    vtkm::Vec3f Dxdydz;

    CoordPortalType Coords;
    FloatPortalType Radii;

    IdPortalType PointIds;
    IdPortalType LowerBounds;
    IdPortalType UpperBounds;
};

#endif // OVERLAP_DETECTOR_H
