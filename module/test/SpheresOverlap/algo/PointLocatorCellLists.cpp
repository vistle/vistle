#include <vtkm/cont/Algorithm.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/PointLocatorSparseGrid.h>

#include <vtkm/worklet/WorkletMapField.h>

#include "PointLocatorCellLists.h"

using namespace vistle;

// TODO: better to avoid duplicate checks (to avoid duplicate lines) or
//       unbalanced thread loads (lower ids have more comparisons than higher ids)
class OverlapDetector {
public:
    // TODO: do we need that? or is arrayhandle<vec> enough?
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

    VTKM_EXEC void CountOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &queryPoint,
                                 vtkm::cont::ArrayHandle<vtkm::Vec2i> &overlaps) const
    {
        vtkm::Id3 ijk = (queryPoint - this->Min) / this->Dxdydz;
        ijk = vtkm::Max(ijk, vtkm::Id3(0));
        ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));

        vtkm::Id cellId = ijk[0] + (ijk[1] * this->Dims[0]) + (ijk[2] * this->Dims[0] * this->Dims[1]);


        // cells of interest
        for (int i = ijk[0]; i <= ijk[0] + 1; i++) {
            for (int j = ijk[1]; j <= ijk[1] + 1; j++) {
                for (int k = ijk[2]; k <= ijk[2]; k++) {
                    auto cellToCheck = i + (j * this->Dims[0]) + (k * this->Dims[0] * this->Dims[1]);
                    if (cellId < cellToCheck) {
                        for (auto p = this->LowerBounds.Get(cellToCheck); p < this->UpperBounds.Get(cellToCheck); p++) {
                            if (pointId < p) {
                                auto toCheck = this->PointIds.Get(p);
                                auto pointToCheck = this->Coords.Get(toCheck);
                                if (auto distance = vtkm::Magnitude(queryPoint - pointToCheck);
                                    (distance < this->Radii.Get(cellId) + this->Radii.Get(toCheck))) {
                                    // TODO: allocating everytime is not nice ...
                                    overlaps.Allocate(overlaps.GetNumberOfValues() + 1, vtkm::CopyFlag::On);
                                    overlaps.WritePortal().Set(overlaps.GetNumberOfValues() - 1, {pointId, p});
                                }
                            }
                        }
                    }
                }
            }
        }
    }

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

class BinPointsWorklet: public vtkm::worklet::WorkletMapField {
public:
    using ControlSignature = void(FieldIn coord, FieldOut label);

    using ExecutionSignature = void(_1, _2);

    VTKM_CONT
    BinPointsWorklet(vtkm::Vec3f min, vtkm::Vec3f max, vtkm::Id3 dims): Min(min), Dims(dims), Dxdydz((max - Min) / Dims)
    {}

    template<typename CoordVecType, typename IdType>
    VTKM_EXEC void operator()(const CoordVecType &coord, IdType &label) const
    {
        vtkm::Id3 ijk = (coord - Min) / Dxdydz;
        ijk = vtkm::Max(ijk, vtkm::Id3(0));
        ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));
        label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
    }

private:
    vtkm::Vec3f Min;
    vtkm::Id3 Dims;
    vtkm::Vec3f Dxdydz;
};

void PointLocatorCellLists::Build()
{
    vtkm::Bounds test;
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "PointLocatorCellLists::Build");

    auto bounds = this->GetCoordinates().GetBounds();

    this->Dims = vtkm::Id3(vtkm::Ceil((bounds.X.Max - bounds.X.Min) / this->SearchRadius),
                           vtkm::Ceil((bounds.Y.Max - bounds.Y.Min) / this->SearchRadius),
                           vtkm::Ceil((bounds.Z.Max - bounds.Z.Min) / this->SearchRadius));

    this->Min = vtkm::make_Vec(bounds.X.Min, bounds.Y.Min, bounds.Z.Min);

    this->Max = vtkm::make_Vec(bounds.X.Min + this->Dims[0] * this->SearchRadius,
                               bounds.Y.Min + this->Dims[1] * this->SearchRadius,
                               bounds.Z.Min + this->Dims[2] * this->SearchRadius);

    // generate unique id for each input point
    vtkm::cont::ArrayHandleIndex pointIndex(this->GetCoordinates().GetNumberOfValues());
    vtkm::cont::ArrayCopy(pointIndex, this->PointIds);

    // bin points into cells and give each of them the cell id.
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds;
    BinPointsWorklet cellIdWorklet(this->Min, this->Max, this->Dims);
    vtkm::cont::Invoker invoke;
    invoke(cellIdWorklet, this->GetCoordinates(), cellIds);

    // Group points of the same cell together by sorting them according to the cell ids
    vtkm::cont::Algorithm::SortByKey(cellIds, this->PointIds);

    // for each cell, find the lower and upper bound of indices to the sorted point ids.
    vtkm::cont::ArrayHandleCounting<vtkm::Id> cell_ids_counting(0, 1, this->Dims[0] * this->Dims[1] * this->Dims[2]);
    vtkm::cont::Algorithm::UpperBounds(cellIds, cell_ids_counting, this->CellUpperBounds);
    vtkm::cont::Algorithm::LowerBounds(cellIds, cell_ids_counting, this->CellLowerBounds);
}

OverlapDetector PointLocatorCellLists::PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                           vtkm::cont::Token &token) const
{
    return OverlapDetector(
        this->Min, this->Max, this->Dims, this->GetCoordinates().GetDataAsMultiplexer().PrepareForInput(device, token),
        this->Radii.PrepareForInput(device, token), this->PointIds.PrepareForInput(device, token),
        this->CellLowerBounds.PrepareForInput(device, token), this->CellUpperBounds.PrepareForInput(device, token));
}
