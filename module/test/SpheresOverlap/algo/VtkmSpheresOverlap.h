#ifndef VTKM_SPHERES_OVERLAP_H
#define VTKM_SPHERES_OVERLAP_H

#include <vtkm/filter/density_estimate/ParticleDensityBase.h>

class VtkmSpheresOverlap: public vtkm::filter::density_estimate::ParticleDensityBase {
public:
    using Superclass = ParticleDensityBase;

    VtkmSpheresOverlap() = default;

private:
    VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet &input) override;
};

#endif //VTKM_SPHERES_OVERLAP_H
