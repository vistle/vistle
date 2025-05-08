#include <algorithm> // for std::max

#include <viskores/cont/Algorithm.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/PointLocatorSparseGrid.h>

#include <viskores/worklet/WorkletMapField.h>

#include "PointLocatorCellLists.h"

using namespace vistle;

class BinPointsWorklet: public viskores::worklet::WorkletMapField {
public:
    using ControlSignature = void(FieldIn coord, FieldOut label);

    using ExecutionSignature = void(_1, _2);

    VISKORES_CONT
    BinPointsWorklet(viskores::Vec3f min, viskores::Vec3f max, viskores::Id3 dims)
    : Min(min), Dims(dims), Dxdydz((max - Min) / Dims)
    {}

    template<typename CoordVecType, typename IdType>
    VISKORES_EXEC void operator()(const CoordVecType &coord, IdType &label) const
    {
        viskores::Id3 ijk = (coord - Min) / Dxdydz;
        ijk = viskores::Max(ijk, viskores::Id3(0));
        ijk = viskores::Min(ijk, this->Dims - viskores::Id3(1));
        label = ijk[0] + ijk[1] * Dims[0] + ijk[2] * Dims[0] * Dims[1];
    }

private:
    viskores::Vec3f Min;
    viskores::Id3 Dims;
    viskores::Vec3f Dxdydz;
};

VISKORES_CONT void PointLocatorCellLists::Build()
{
    VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "PointLocatorCellLists::Build");

    auto bounds = this->GetCoordinates().GetBounds();

    auto vtkmMax = viskores::Maximum();

    // to make sure overlaps can be found for spheres defined on 1D or 2D grids, too, dimensions have to be at least 1
    this->Dims = viskores::Id3(vtkmMax(1.0, viskores::Ceil((bounds.X.Max - bounds.X.Min) / this->SearchRadius)),
                               vtkmMax(1.0, viskores::Ceil((bounds.Y.Max - bounds.Y.Min) / this->SearchRadius)),
                               vtkmMax(1.0, viskores::Ceil((bounds.Z.Max - bounds.Z.Min) / this->SearchRadius)));

    this->Min = viskores::make_Vec(bounds.X.Min, bounds.Y.Min, bounds.Z.Min);

    this->Max = viskores::make_Vec(bounds.X.Min + this->Dims[0] * this->SearchRadius,
                                   bounds.Y.Min + this->Dims[1] * this->SearchRadius,
                                   bounds.Z.Min + this->Dims[2] * this->SearchRadius);

    // generate unique id for each input point
    viskores::cont::ArrayHandleIndex pointIndex(this->GetCoordinates().GetNumberOfValues());
    viskores::cont::ArrayCopy(pointIndex, this->PointIds);

    // bin points into cells and give each of them the cell id.
    viskores::cont::ArrayHandle<viskores::Id> cellIds;
    BinPointsWorklet cellIdWorklet(this->Min, this->Max, this->Dims);
    viskores::cont::Invoker invoke;
    invoke(cellIdWorklet, this->GetCoordinates(), cellIds);

    // Group points of the same cell together by sorting them according to the cell ids
    viskores::cont::Algorithm::SortByKey(cellIds, this->PointIds);

    // for each cell, find the lower and upper bound of indices to the sorted point ids.
    viskores::cont::ArrayHandleCounting<viskores::Id> cell_ids_counting(0, 1,
                                                                        this->Dims[0] * this->Dims[1] * this->Dims[2]);
    viskores::cont::Algorithm::UpperBounds(cellIds, cell_ids_counting, this->CellUpperBounds);
    viskores::cont::Algorithm::LowerBounds(cellIds, cell_ids_counting, this->CellLowerBounds);

    this->OffsetsToNeighbors = viskores::cont::make_ArrayHandle<viskores::Int8>({
        -1, -1, -1, -1, -1, 0, -1, -1, 1, -1, 0, -1, -1, 0, 0, -1, 0, 1, -1, 1, -1, -1, 1, 0, -1, 1, 1,
        0,  -1, -1, 0,  -1, 0, 0,  -1, 1, 0,  0, -1, 0,  0, 0, 0,  0, 1, 0,  1, -1, 0,  1, 0, 0,  1, 1,
        1,  -1, -1, 1,  -1, 0, 1,  -1, 1, 1,  0, -1, 1,  0, 0, 1,  0, 1, 1,  1, -1, 1,  1, 0, 1,  1, 1,

    });
}

OverlapDetector PointLocatorCellLists::PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                                           viskores::cont::Token &token) const
{
    return OverlapDetector(
        this->Min, this->Max, this->Dims, this->GetCoordinates().GetDataAsMultiplexer().PrepareForInput(device, token),
        this->Radii.PrepareForInput(device, token), this->PointIds.PrepareForInput(device, token),
        this->CellLowerBounds.PrepareForInput(device, token), this->CellUpperBounds.PrepareForInput(device, token),
        this->Determiner, this->OffsetsToNeighbors.PrepareForInput(device, token));
}
