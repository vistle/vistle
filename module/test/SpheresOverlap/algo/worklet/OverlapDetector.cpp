#include "OverlapDetector.h"

VTKM_EXEC void OverlapDetector::DetectOverlaps(const vtkm::Id pointId, const vtkm::Vec3f &queryPoint,
                                               vtkm::cont::ArrayHandle<vtkm::Vec2i> &overlaps) const
{
    /*vtkm::Id3 ijk = (queryPoint - this->Min) / this->Dxdydz;
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
    }*/
}