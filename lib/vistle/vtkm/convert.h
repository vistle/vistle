#ifndef VISTLE_VTKM_CONVERT_H
#define VISTLE_VTKM_CONVERT_H

// transform a vistle dataset (grid and scalar data field) into a vtkm dataset
// so vtkm filters can be applied to it

#include <vistle/core/object.h>
#include <vistle/core/database.h>

#include <vtkm/cont/DataSet.h>

#include "module_status.h"
#include "export.h"

namespace vistle {

// transform a vistle grid object and transfer it a vtkm dataset
ModuleStatusPtr V_VTKM_EXPORT vtkmSetGrid(vtkm::cont::DataSet &vtkmDataSet, vistle::Object::const_ptr grid);

// transform a vistle field object and add it to a vtkm dataset as 'name'
ModuleStatusPtr V_VTKM_EXPORT vtkmAddField(vtkm::cont::DataSet &vtkmDataSet, const vistle::DataBase::const_ptr &field,
                                           const std::string &name,
                                           vistle::DataBase::Mapping mapping = vistle::DataBase::Unspecified);

// retrieve the geometry from a vtkm dataset as a vistle object
vistle::Object::ptr V_VTKM_EXPORT vtkmGetGeometry(const vtkm::cont::DataSet &dataset);

// retrieve field 'name' from a vtkm dataset in vistle format
vistle::DataBase::ptr V_VTKM_EXPORT vtkmGetField(const vtkm::cont::DataSet &vtkmDataSet, const std::string &name);


} // namespace vistle

#endif
