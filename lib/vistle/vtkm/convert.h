#ifndef VISTLE_VTKM_UTILS_H
#define VISTLE_VTKM_UTILS_H

#include <vistle/core/object.h>
#include <vistle/core/scalar.h>
#include <vistle/core/triangles.h>
#include <vistle/core/vec.h>

#include <vtkm/cont/DataSet.h>

#include "export.h"

namespace vistle {

enum VtkmTransformStatus { SUCCESS = 0, UNSUPPORTED_GRID_TYPE = 1, UNSUPPORTED_CELL_TYPE = 2, UNSUPPORTED_FIELD_TYPE };

// transform a vistle dataset (grid and scalar data field) into a vtkm dataset
// so vtkm filters can be applied to it
VtkmTransformStatus V_VTKM_EXPORT vtkmSetGrid(vtkm::cont::DataSet &vtkmDataSet, vistle::Object::const_ptr grid);

VtkmTransformStatus V_VTKM_EXPORT vtkmAddField(vtkm::cont::DataSet &vtkmDataSet,
                                               const vistle::DataBase::const_ptr &field, const std::string &name,
                                               vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified);

// transform a vtkm isosurface dataset into a vistle Triangles object, so it can
// be rendered in COVER
vistle::Object::ptr V_VTKM_EXPORT vtkmGetGeometry(vtkm::cont::DataSet &dataset);

vistle::DataBase::ptr V_VTKM_EXPORT vtkmGetField(const vtkm::cont::DataSet &vtkmDataSet, const std::string &name);

} // namespace vistle

#endif // VISTLE_VTKM_UTILS_H
