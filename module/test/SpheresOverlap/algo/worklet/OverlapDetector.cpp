#include <vtkm/VectorAnalysis.h>

#include "OverlapDetector.h"

VTKM_EXEC void OverlapDetector::CountOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &point,
                                              vtkm::Id &nrOverlaps) const
{
    vtkm::Id3 ijk = (point - this->Min) / this->Dxdydz;
    ijk = vtkm::Max(ijk, vtkm::Id3(0));
    ijk = vtkm::Min(ijk, this->Dims - vtkm::Id3(1));

    vtkm::Id cellId = ijk[0] + (ijk[1] * this->Dims[0]) + (ijk[2] * this->Dims[0] * this->Dims[1]);

    nrOverlaps = 0;
    for (int i = ijk[0] - 1; i <= ijk[0] + 1; i++) {
        for (int j = ijk[1] - 1; j <= ijk[1] + 1; j++) {
            for (int k = ijk[2] - 1; k <= ijk[2] + 1; k++) {
                auto cellToCheck = i + (j * this->Dims[0]) + (k * this->Dims[0] * this->Dims[1]);
                if (cellToCheck < this->LowerBounds.GetNumberOfValues() && cellId < cellToCheck) {
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