#ifndef VISTLE_VTK_VTKTOVISTLE_H
#define VISTLE_VTK_VTKTOVISTLE_H

class vtkDataSet;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkDataObject;
class vtkDataArray;
class vtkInformation;
class vtkImageData;

#include "export.h"

#include <string>
#include <vector>

#include <vistle/core/object.h>
#include <vistle/core/database.h>


namespace vistle {
namespace vtk {

Object::ptr V_VTK_EXPORT toGrid(vtkDataObject *ds, std::string *diagnostics = nullptr);
DataBase::ptr V_VTK_EXPORT getField(vtkDataSetAttributes *ds, const std::string &name,
                                    Object::const_ptr grid = Object::const_ptr(), std::string *diagnostics = nullptr);
DataBase::ptr V_VTK_EXPORT getField(vtkFieldData *ds, const std::string &name,
                                    Object::const_ptr grid = Object::const_ptr(), std::string *diagnostics = nullptr);

} // namespace vtk
} // namespace vistle

#endif
