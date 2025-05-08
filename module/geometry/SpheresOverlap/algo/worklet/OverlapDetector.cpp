#include <viskores/VectorAnalysis.h>

#include "OverlapDetector.h"

VISKORES_EXEC viskores::Id3 OverlapDetector::DetermineCellId(const viskores::Vec3f &point) const
{
    viskores::Id3 ijk = (point - this->Min) / this->Dxdydz;
    ijk = viskores::Max(ijk, viskores::Id3(0));
    ijk = viskores::Min(ijk, this->Dims - viskores::Id3(1));

    return ijk;
}

VISKORES_EXEC bool OverlapDetector::CellExists(const viskores::Id3 &id) const
{
    return id[0] >= 0 && id[0] < this->Dims[0] && id[1] >= 0 && id[1] < this->Dims[1] && id[2] >= 0 &&
           id[2] < this->Dims[2];
}

VISKORES_EXEC viskores::Id OverlapDetector::FlattenCellId(const viskores::Id3 &id) const
{
    if (!CellExists(id))
        return -1;
    return id[0] + (id[1] * this->Dims[0]) + (id[2] * this->Dims[0] * this->Dims[1]);
}

/*
    Returns the number of points in the dataset that overlap with the point `point` at the index `pointId`.
*/
VISKORES_EXEC void OverlapDetector::CountOverlaps(const viskores::Id pointId, const viskores::Vec3f &point,
                                                  viskores::Id &nrOverlaps) const
{
    auto cellId3 = this->DetermineCellId(point);
    auto cellId = FlattenCellId(cellId3);

    nrOverlaps = 0;

    // loop through the cell the current point lies in as well as its neighbor cells
    for (int i = 0; i < this->OffsetsToNeighbors.GetNumberOfValues(); i += 3) {
        auto cellToCheck = FlattenCellId({cellId3[0] + this->OffsetsToNeighbors.Get(i),
                                          cellId3[1] + this->OffsetsToNeighbors.Get(i + 1),
                                          cellId3[2] + this->OffsetsToNeighbors.Get(i + 2)});

        if (cellToCheck != -1 && cellId <= cellToCheck) { // avoid checking the same pair of cells twice
            // get the ids of all points in the cell
            for (auto boundId = this->LowerBounds.Get(cellToCheck); boundId < this->UpperBounds.Get(cellToCheck);
                 boundId++) {
                auto idToCheck = this->PointIds.Get(boundId);

                // if two overlapping spheres lie in the same cell, we can avoid checking the same pair twice
                // by only checking the point with the "smaller" id for overlap (note that the points are not sorted)
                if ((cellId != cellToCheck && pointId != idToCheck) || (cellId == cellToCheck && pointId < idToCheck)) {
                    auto pointToCheck = this->Coords.Get(idToCheck);

                    // check if spheres overlap (here it's enough to calculate the squared distance)
                    if (auto distance = viskores::MagnitudeSquared(point - pointToCheck);
                        (distance <= viskores::Pow(this->Radii.Get(pointId) + this->Radii.Get(idToCheck), 2))) {
                        nrOverlaps++;
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
VISKORES_EXEC void OverlapDetector::CreateConnectionLines(const viskores::Id pointId, const viskores::Vec3f &point,
                                                          const viskores::IdComponent visitId,
                                                          viskores::Id2 &connectivity,
                                                          viskores::FloatDefault &thickness) const
{
    auto cellId3 = this->DetermineCellId(point);
    auto cellId = FlattenCellId(cellId3);

    viskores::Id nrOverlaps = 0;

    bool foundOverlap = false;
    int i = 0;

    while (!foundOverlap) {
        auto cellToCheck = FlattenCellId({cellId3[0] + this->OffsetsToNeighbors.Get(i),
                                          cellId3[1] + this->OffsetsToNeighbors.Get(i + 1),
                                          cellId3[2] + this->OffsetsToNeighbors.Get(i + 2)});
        if (cellToCheck != -1 && cellId <= cellToCheck) { // avoid checking the same pair of cells twice
            // get the ids of all points in the cell
            for (auto boundId = this->LowerBounds.Get(cellToCheck); boundId < this->UpperBounds.Get(cellToCheck);
                 boundId++) {
                auto idToCheck = this->PointIds.Get(boundId);

                // if two overlapping spheres lie in the same cell, we can avoid checking the same pair twice
                // by only checking the point with the "smaller" id for overlap (note that the points are not sorted)
                if ((cellId != cellToCheck && pointId != idToCheck) || (cellId == cellToCheck && pointId < idToCheck)) {
                    auto pointToCheck = this->Coords.Get(idToCheck);

                    // check if spheres overlap
                    auto radius1 = this->Radii.Get(pointId);
                    auto radius2 = this->Radii.Get(idToCheck);
                    if (auto distance = viskores::Magnitude(point - pointToCheck); (distance <= radius1 + radius2)) {
                        if (nrOverlaps == visitId) {
                            connectivity[0] = pointId;
                            connectivity[1] = idToCheck;
                            thickness = CalculateThickness(this->Determiner, distance, radius1, radius2);
                            foundOverlap = true;
                            break;
                        }
                        nrOverlaps++;
                    }
                }
            }
        }
        i += 3;
    }
}
