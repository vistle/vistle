#ifndef VISTLE_VTKM_UTILS_H
#define VISTLE_VTKM_UTILS_H

#include <vistle/core/object.h>
#include <vistle/core/scalar.h>
#include <vistle/core/triangles.h>
#include <vistle/core/vec.h>

#include <vtkm/cont/DataSet.h>

#include "export.h"
#include "transformEnums.h"

namespace vistle {

// converts a vistle geometry (i.e., its cellset and coordinates) into a vtkm dataset
VtkmTransformStatus V_VTKM_EXPORT geometryToVtkm(Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset);

// converts a vistle data field to a vtkm field which will be added to `vtkmDataset` with the field name `fieldName`
VtkmTransformStatus V_VTKM_EXPORT fieldToVtkm(const DataBase::const_ptr &field, vtkm::cont::DataSet &vtkmDataset,
                                              const std::string &fieldName, vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified);

// converts a vtkm geometry (i.e., its cellset and coordinates) into a vistle geometry
Object::ptr V_VTKM_EXPORT vtkmGeometryToVistle(vtkm::cont::DataSet &vtkmDataset);

// converts a vtkm field in `vtkmDataset` with the name `fieldName` into a vistle field that can be added to a vistle geometry
DataBase::ptr V_VTKM_EXPORT vtkmFieldToVistle(const vtkm::cont::DataSet &vtkmDataset, const std::string &fieldName);

} // namespace vistle

#endif // VISTLE_VTKM_UTILS_H
