#ifndef VTKM_SPHERES_OVERLAP_H
#define VTKM_SPHERES_OVERLAP_H

#include <vtkm/filter/FilterField.h>

class VtkmSpheresOverlap: public vtkm::filter::FilterField {
public:
    VtkmSpheresOverlap() = default;
    

private:
    VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet &input) override;
};

#endif //VTKM_SPHERES_OVERLAP_H
