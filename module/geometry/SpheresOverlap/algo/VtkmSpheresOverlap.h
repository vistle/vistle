#ifndef VISTLE_SPHERESOVERLAP_ALGO_VTKMSPHERESOVERLAP_H
#define VISTLE_SPHERESOVERLAP_ALGO_VTKMSPHERESOVERLAP_H

#include <viskores/filter/Filter.h>

#include "ThicknessDeterminer.h"

#include "worklet/PointLocatorCellLists.h"

/*
    Detects overlapping spheres and creates connections lines between them.
    To efficiently compare all spheres with each other, the Cell Lists Algorithm is used.
*/
class VtkmSpheresOverlap: public viskores::filter::Filter {
public:
    VtkmSpheresOverlap() = default;

    /*
        By default, the search radius is set the maximum radius in the dataset times 2.
        You can add a factor that will be multiplied to this search radius with this setter.
    */
    void SetRadiusFactor(viskores::FloatDefault factor)
    {
        assert(factor > 0);
        this->RadiusFactor = factor;
    }

    void SetThicknessDeterminer(ThicknessDeterminer determiner) { this->Determiner = determiner; }

private:
    viskores::FloatDefault RadiusFactor = 1;
    ThicknessDeterminer Determiner = OverlapRatio;

    VISKORES_CONT PointLocatorCellLists CreateSearchGrid(const viskores::cont::CoordinateSystem &spheresCoords,
                                                         const viskores::cont::Field &radii);
    VISKORES_CONT viskores::cont::ArrayHandle<viskores::Id>
    CountOverlapsPerPoint(const PointLocatorCellLists &pointLocator);
    VISKORES_CONT void CreateConnectionLines(const PointLocatorCellLists &pointLocator,
                                             const viskores::cont::ArrayHandle<viskores::Id> &overlapsPerPoint,
                                             viskores::cont::ArrayHandle<viskores::Id> &linesConnectivity,
                                             viskores::cont::ArrayHandle<viskores::FloatDefault> &linesThicknesses);

    VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet &input) override;
};

#endif
