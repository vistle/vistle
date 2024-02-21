#ifndef VTKM_SPHERES_OVERLAP_H
#define VTKM_SPHERES_OVERLAP_H

#include <vtkm/filter/FilterField.h>

#include "worklet/PointLocatorCellLists.h"
#include "ThicknessDeterminer.h"

/*
    Detects overlapping spheres and creates connections lines between them.
    To efficiently compare all spheres with each other, the Cell Lists Algorithm is used.
*/
class VtkmSpheresOverlap: public vtkm::filter::FilterField {
public:
    VtkmSpheresOverlap() = default;

    /*
        By default, the search radius is set the maximum radius in the dataset times 2.
        You can add a factor that will be multiplied to this search radius with this setter.
    */
    void SetRadiusFactor(vtkm::FloatDefault factor)
    {
        assert(factor > 0);
        this->RadiusFactor = factor;
    }

    void SetThicknessDeterminer(ThicknessDeterminer determiner) { this->Determiner = determiner; }

private:
    vtkm::FloatDefault RadiusFactor = 1;
    ThicknessDeterminer Determiner = OverlapRatio;
    
    VTKM_CONT PointLocatorCellLists CreateSearchGrid(const vtkm::cont::CoordinateSystem &spheresCoords,
                                                     const vtkm::cont::Field &radii);
    VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> CountOverlapsPerPoint(const PointLocatorCellLists &pointLocator);
    VTKM_CONT void CreateConnectionLines(const PointLocatorCellLists &pointLocator,
                                         const vtkm::cont::ArrayHandle<vtkm::Id> &overlapsPerPoint,
                                         vtkm::cont::ArrayHandle<vtkm::Id> &linesConnectivity,
                                         vtkm::cont::ArrayHandle<vtkm::FloatDefault> &linesThicknesses);

    VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet &input) override;
};

#endif //VTKM_SPHERES_OVERLAP_H
