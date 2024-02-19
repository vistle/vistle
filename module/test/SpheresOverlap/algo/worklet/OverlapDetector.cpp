#include <vtkm/VectorAnalysis.h>

#include "OverlapDetector.h"

// TODO:  - make sure lib/vistle/vtkm/convert.cpp still works the same!
//        - find out why overlapRatio is zero for 2x2x2 grid on (0, 0, 0) -> (1,1,1)

VTKM_EXEC vtkm::Id3 OverlapDetector::GetCellId(const vtkm::Vec3f &point) const
{
    vtkm::Id3 ijk = (point - this->Min) / this->Dxdydz;
    ijk = vtkm::Max(ijk, vtkm::Id3(0));
    ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));

    return ijk;
}

VTKM_EXEC bool OverlapDetector::CellExists(const vtkm::Id3 &id) const
{
    return id[0] >= 0 && id[0] < this->Dims[0] && id[1] >= 0 && id[1] < this->Dims[1] && id[2] >= 0 &&
           id[2] < this->Dims[2];
}

VTKM_EXEC vtkm::Id OverlapDetector::FlattenId(const vtkm::Id3 &id) const
{
    if (!CellExists(id))
        return -1;
    return id[0] + (id[1] * this->Dims[0]) + (id[2] * this->Dims[0] * this->Dims[1]);
}

/*
    Returns the number of points in the dataset that overlap with the point `point` at the index `pointId`.
*/
VTKM_EXEC void OverlapDetector::CountOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &point,
                                              vtkm::Id &nrOverlaps) const
{
    auto cellId3 = this->GetCellId(point);
    auto cellId = FlattenId(cellId3);

    nrOverlaps = 0;
    // loop through the cell the current point lies in as well as its neighbor cells
    for (int i = cellId3[0] - 1; i <= cellId3[0] + 1; i++) {
        for (int j = cellId3[1] - 1; j <= cellId3[1] + 1; j++) {
            for (int k = cellId3[2] - 1; k <= cellId3[2] + 1; k++) {
                auto cellToCheck = FlattenId({i, j, k});

                if (cellToCheck != -1 && cellId <= cellToCheck) { // avoid checking the same pair of cells twice
                    // get the ids of all points in the cell
                    for (auto boundId = this->LowerBounds.Get(cellToCheck);
                         boundId < this->UpperBounds.Get(cellToCheck); boundId++) {
                        auto idToCheck = this->PointIds.Get(boundId);

                        if (pointId < idToCheck) { // avoid checking the same pair of points twice
                            auto pointToCheck = this->Coords.Get(idToCheck);

                            // check if spheres overlap
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

/*
    Determines which points overlap with the point `point` at the index `pointId` and for each overlap
    saves the indices of the two points into `connectivity`. and a scalar value into `thickness`. 
*/
VTKM_EXEC void OverlapDetector::CreateOverlapLines(const vtkm::Id pointId, const vtkm::Vec3f &point,
                                                   const vtkm::IdComponent visitId, vtkm::Id2 &connectivity,
                                                   vtkm::FloatDefault &thickness) const
{
    auto cellId3 = this->GetCellId(point);
    auto cellId = FlattenId(cellId3);

    vtkm::Id nrOverlaps = 0;
    // loop through the cell the current point lies in as well as its neighbor cells
    for (int i = cellId3[0] - 1; i <= cellId3[0] + 1; i++) {
        for (int j = cellId3[1] - 1; j <= cellId3[1] + 1; j++) {
            for (int k = cellId3[2] - 1; k <= cellId3[2] + 1; k++) {
                auto cellToCheck = FlattenId({i, j, k});

                if (cellToCheck != -1 && cellId <= cellToCheck) { // avoid checking the same pair of cells twice
                    // get the ids of all points in the cell
                    for (auto boundId = this->LowerBounds.Get(cellToCheck);
                         boundId < this->UpperBounds.Get(cellToCheck); boundId++) {
                        auto idToCheck = this->PointIds.Get(boundId);

                        if (pointId < idToCheck) { // avoid checking the same pair of points twice
                            auto pointToCheck = this->Coords.Get(idToCheck);

                            // check if spheres overlap
                            auto radius1 = this->Radii.Get(pointId);
                            auto radius2 = this->Radii.Get(idToCheck);
                            if (auto distance = vtkm::Magnitude(point - pointToCheck);
                                (distance <= radius1 + radius2)) {
                                if (nrOverlaps == visitId) {
                                    connectivity[0] = pointId;
                                    connectivity[1] = idToCheck;
                                    thickness = CalculateThickness(this->Determiner, distance, radius1, radius2);
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
