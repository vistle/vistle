/* This file is part of Vistle.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

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

#ifdef SENSEI //for sensei we need an extra argument to crete Vistle objects safely (with synced shm ids)
#define SENSEI_ARGUMENT vistle::insitu::sensei::SenseiAdapter &senseiAdapter,
#define SENSEI_PARAMETER senseiAdapter,
#else
#define SENSEI_ARGUMENT
#define SENSEI_PARAMETER
#endif // SENSEI


namespace vistle {
#ifdef SENSEI
namespace insitu {
namespace sensei {
class SenseiAdapter;
}
} // namespace insitu
#endif
namespace vtk {

Object::ptr V_VTK_EXPORT toGrid(SENSEI_ARGUMENT vtkDataObject *ds, bool checkConvex = false);
DataBase::ptr V_VTK_EXPORT getField(SENSEI_ARGUMENT vtkDataSetAttributes *ds, const std::string &name,
                                    Object::const_ptr grid = Object::const_ptr());
DataBase::ptr V_VTK_EXPORT getField(SENSEI_ARGUMENT vtkFieldData *ds, const std::string &name,
                                    Object::const_ptr grid = Object::const_ptr());
#ifdef SENSEI
DataBase::ptr V_VTK_EXPORT vtkData2Vistle(SENSEI_ARGUMENT vtkDataArray *varr, Object::const_ptr grid);
#endif
} // namespace vtk
} // namespace vistle

class coVtk {
public:
    enum Attributes { Scalars = 0, Vectors = 1, Normals = 2, TexCoords = 3, Tensors = 4, Any = 5 };

    enum Flags { None = 0, Normalize = 1, RequireDouble = 2 };
};

#endif
