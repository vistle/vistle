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

// transform a vistle grid (i.e., its cellset and coordinates) into a vtkm dataset
VtkmTransformStatus V_VTKM_EXPORT gridToVtkm(vistle::Object::const_ptr grid, vtkm::cont::DataSet &vtkmDataset);

VtkmTransformStatus V_VTKM_EXPORT vtkmAddField(vtkm::cont::DataSet &vtkmDataset,
                                               const vistle::DataBase::const_ptr &field, const std::string &name);

// transform a vtkm isosurface dataset into a vistle Triangles object, so it can
// be rendered in COVER
vistle::Object::ptr V_VTKM_EXPORT vtkmGetGeometry(vtkm::cont::DataSet &dataset);

vistle::DataBase::ptr V_VTKM_EXPORT vtkmGetField(const vtkm::cont::DataSet &vtkmDataset, const std::string &name);

} // namespace vistle

#endif // VISTLE_VTKM_UTILS_H
