#ifndef POINT_LOCATOR_CELL_LISTS_H
#define POINT_LOCATOR_CELL_LISTS_H

#include <vtkm/cont/internal/PointLocatorBase.h>

#include <vistle/core/scalar.h>

class PointLocatorCellLists: public vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists> {
    using Superclass = vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists>;

public:
    void SetBounds(const vtkm::Bounds &bounds)
    {
        if (this->bounds != bounds) {
            assert((bounds.X.Min < bounds.X.Max || bounds.Y.Min < bounds.Y.Max || bounds.Z.Min < bounds.Z.Max));
            this->bounds = bounds;
            this->SetModified();
        }
    }

    void SetNumberOfBins(const vtkm::Id3 &nrBins)
    {
        if (this->nrBins != nrBins) {
            this->nrBins = nrBins;
            this->SetModified();
        }
    }


    const vtkm::Bounds &GetBounds() const { return this->bounds; }
    const vtkm::Id3 &GetNumberOfBins() const { return this->nrBins; }

private:

    vtkm::Bounds bounds;
    vtkm::Id3 nrBins;

    // stores which sphere center is located in which cell
    vtkm::cont::ArrayHandle<vtkm::cont::ArrayHandle<vistle::Scalar>> cellLists;


    // create the search grid
    VTKM_CONT void Build();

    friend Superclass;
};

#endif // POINT_LOCATOR_CELL_LISTS_H
