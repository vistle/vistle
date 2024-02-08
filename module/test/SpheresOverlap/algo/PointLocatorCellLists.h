#ifndef POINT_LOCATOR_CELL_LISTS_H
#define POINT_LOCATOR_CELL_LISTS_H

#include <vtkm/cont/internal/PointLocatorBase.h>

#include <vistle/core/scalar.h>

class OverlapDetector;

/*
    TODO: - pass radius
          - store line thickness
*/
class PointLocatorCellLists: public vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists> {
    using Superclass = vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists>;

public:
    void SetRadii(const vtkm::cont::UnknownArrayHandle& radii){
        assert(radii.CanConvert<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>());
        this->Radii = radii.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>();
    }

    void SetSearchRadius(const vtkm::FloatDefault searchRadius)
    {
        if (this->SearchRadius != searchRadius) {
            assert(SearchRadius > 0);
            this->SearchRadius = searchRadius;
            this->SetModified();
        }
    }

    const vtkm::Vec3f &GetMinPoint() const { return this->Min; }
    const vtkm::Vec3f &GetMaxPoint() const { return this->Max; }
    const vtkm::Id3 &GetNumberOfBins() const { return this->Dims; }
    const vtkm::FloatDefault &GetSearchRadius() const { return this->SearchRadius; }

    VTKM_CONT
    OverlapDetector PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token &token) const;

private:
    // Search grid properties
    vtkm::Vec3f Min, Max;
    vtkm::Id3 Dims;
    vtkm::FloatDefault SearchRadius;

    // Cell lists
    // a list of all point ids (grouped by the cells that contain them)
    vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
    // entry `i` stores the first index into `PointsIds` which corresponds to a point lying in cell `i`
    vtkm::cont::ArrayHandle<vtkm::Id> CellLowerBounds;
    // entry `i` stores the last index into `PointsIds` which corresponds to a point lying in cell `i`
    vtkm::cont::ArrayHandle<vtkm::Id> CellUpperBounds;

    // point field containing sphere radii
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Radii;

    // create the search grid
    VTKM_CONT void Build();

    friend Superclass;
};

#endif // POINT_LOCATOR_CELL_LISTS_H
