#ifndef VTKM_SPHERES_OVERLAP_H
#define VTKM_SPHERES_OVERLAP_H

#include <vtkm/filter/FilterField.h>

#include "ThicknessDeterminer.h"

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
    VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet &input) override;

    vtkm::FloatDefault RadiusFactor = 1;
    ThicknessDeterminer Determiner = OverlapRatio;
};

#endif //VTKM_SPHERES_OVERLAP_H
