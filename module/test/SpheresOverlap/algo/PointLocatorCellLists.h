#ifndef POINT_LOCATOR_CELL_LISTS_H
#define POINT_LOCATOR_CELL_LISTS_H

#include <vtkm/cont/internal/PointLocatorBase.h>

#include <vistle/core/scalar.h>

class PointLocatorOverlap;

/*
    While vtkm::cont::PointLocatorSparseGrid builds a unifrom search grid than can be
    used to implement the Cell Lists Algorithm, it uses the grid to find a point's nearest
    neighbor in parallel (vtkm::exec::PointLocatorSparseGrid).
    We, however, want to use to efficiently detect all overlapping spheres in a dataset.
    To avoid making changes in vtk-m, we thus copied vtkm::cont::PointLocatorSparseGrid
    and vtkm::exec::PointLocatorSparseGrid and slightly modified them for our needs.
*/
class PointLocatorCellLists: public vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists> {
    using Superclass = vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists>;

public:
    void SetBounds(const vtkm::Bounds &bounds)
    {
        if (this->Bounds != bounds) {
            assert((bounds.X.Min < bounds.X.Max || bounds.Y.Min < bounds.Y.Max || bounds.Z.Min < bounds.Z.Max));
            this->Bounds = bounds;
            this->SetModified();
        }
    }

    void SetNumberOfBins(const vtkm::Id3 &nrBins)
    {
        if (this->NrBins != nrBins) {
            this->NrBins = nrBins;
            this->SetModified();
        }
    }

    const vtkm::Bounds &GetBounds() const { return this->Bounds; }
    const vtkm::Id3 &GetNumberOfBins() const { return this->NrBins; }

    VTKM_CONT
    PointLocatorOverlap PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token &token) const;

private:
    vtkm::Bounds Bounds;
    vtkm::Id3 NrBins;

    // a list of all point ids (grouped by the cells that contain them)
    vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
    // entry `i` stores the first index into `PointsIds` which corresponds to a point lying in cell `i`
    vtkm::cont::ArrayHandle<vtkm::Id> CellLowerBounds;
    // entry `i` stores the last index into `PointsIds` which corresponds to a point lying in cell `i`
    vtkm::cont::ArrayHandle<vtkm::Id> CellUpperBounds;

    // create the search grid
    VTKM_CONT void Build();

    friend Superclass;
};

#endif // POINT_LOCATOR_CELL_LISTS_H
