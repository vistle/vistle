#include <vtkm/VectorAnalysis.h>

#include "OverlapDetector.h"

// TODO:  - make these helper fuctions friends...
//        - make sure lib/vistle/vtkm/convert.cpp still works the same!

VTKM_EXEC vtkm::Id3 OverlapDetector::GetCellId(const vtkm::Vec3f &point) const
{
    vtkm::Id3 ijk = (point - this->Min) / this->Dxdydz;
    ijk = vtkm::Max(ijk, vtkm::Id3(0));
    ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));

    return ijk;
}

VTKM_EXEC bool OverlapDetector::cellExists(const vtkm::Id3 &id) const
{
    return id[0] >= 0 && id[0] < this->Dims[0] && id[1] >= 0 && id[1] < this->Dims[1] && id[2] >= 0 &&
           id[0] < this->Dims[2];
}

VTKM_EXEC vtkm::Id OverlapDetector::FlattenId(const vtkm::Id3 &id) const
{
    if (!cellExists(id))
        return -1;
    return id[0] + (id[1] * this->Dims[0]) + (id[2] * this->Dims[0] * this->Dims[1]);
}

VTKM_EXEC void OverlapDetector::CountOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &point,
                                              vtkm::Id &nrOverlaps) const
{
    auto cellId = this->GetCellId(point);

    nrOverlaps = 0;
    for (int i = cellId[0] - 1; i <= cellId[0] + 1; i++) {
        for (int j = cellId[1] - 1; j <= cellId[1] + 1; j++) {
            for (int k = cellId[2] - 1; k <= cellId[2] + 1; k++) {
                auto cellToCheck = FlattenId({i, j, k});
                if (cellToCheck != -1 && FlattenId(cellId) <= cellToCheck) {
                    for (auto boundId = this->LowerBounds.Get(cellToCheck);
                         boundId < this->UpperBounds.Get(cellToCheck); boundId++) {
                        auto idToCheck = this->PointIds.Get(boundId);
                        if (pointId < idToCheck) {
                            auto pointToCheck = this->Coords.Get(idToCheck);
                            if (auto distance = vtkm::Magnitude(point - pointToCheck);
                                (distance <= this->Radii.Get(pointId) + this->Radii.Get(idToCheck))) {
                                nrOverlaps++;
                            }
                        }
                    }
                }
            }
        }
    }
}

VTKM_EXEC void OverlapDetector::CreateOverlapLines(const vtkm::Id pointId, const vtkm::Vec3f &point,
                                                   const vtkm::IdComponent visitId, vtkm::Id2 &connectivity,
                                                   vtkm::FloatDefault &thickness) const
{
    auto cellId = this->GetCellId(point);

    vtkm::Id nrOverlaps = 0;
    for (int i = cellId[0] - 1; i <= cellId[0] + 1; i++) {
        for (int j = cellId[1] - 1; j <= cellId[1] + 1; j++) {
            for (int k = cellId[2] - 1; k <= cellId[2] + 1; k++) {
                auto cellToCheck = FlattenId({i, j, k});
                if (cellToCheck != -1 && FlattenId(cellId) <= cellToCheck) {
                    for (auto boundId = this->LowerBounds.Get(cellToCheck);
                         boundId < this->UpperBounds.Get(cellToCheck); boundId++) {
                        auto idToCheck = this->PointIds.Get(boundId);
                        if (pointId < idToCheck) {
                            auto pointToCheck = this->Coords.Get(idToCheck);
                            if (auto distance = vtkm::Magnitude(point - pointToCheck);
                                (distance <= this->Radii.Get(pointId) + this->Radii.Get(idToCheck))) {
                                if (nrOverlaps == visitId) {
                                    connectivity[0] = pointId;
                                    connectivity[1] = idToCheck;
                                    thickness = distance;
                                }
                                nrOverlaps++;
                            }
                        }
                    }
                }
            }
        }
    }
}
