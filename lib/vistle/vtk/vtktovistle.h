/* This file is part of Vistle.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

 * License: LGPL 2+ */

#ifndef VTK2VISTLE_H
#define VTK2VISTLE_H

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
#ifdef SENSEI
namespace insitu {
namespace sensei {
class SenseiAdapter;
}
} // namespace insitu
#endif
namespace vtk {

Object::ptr V_VTK_EXPORT toGrid(vtkDataObject *ds, std::string *diagnostics = nullptr);
DataBase::ptr V_VTK_EXPORT getField(vtkDataSetAttributes *ds, const std::string &name,
                                    Object::const_ptr grid = Object::const_ptr(), std::string *diagnostics = nullptr);
DataBase::ptr V_VTK_EXPORT getField(vtkFieldData *ds, const std::string &name,
                                    Object::const_ptr grid = Object::const_ptr(), std::string *diagnostics = nullptr);
#ifdef SENSEI
DataBase::ptr V_VTK_EXPORT vtkData2Vistle(vtkDataArray *varr, Object::const_ptr grid, std::string &diagnostics);
#endif
} // namespace vtk
} // namespace vistle

#endif
