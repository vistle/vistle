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

namespace vistle {
namespace vtk {

Object::ptr V_VTK_EXPORT toGrid(vtkDataObject *ds, bool checkConvex = false);
DataBase::ptr V_VTK_EXPORT getField(vtkDataSetAttributes *ds, const std::string &name, Object::const_ptr grid=Object::const_ptr());
DataBase::ptr V_VTK_EXPORT getField(vtkFieldData *ds, const std::string &name, Object::const_ptr grid=Object::const_ptr());

} // namespace vtk
} // namespace vistle

class coVtk
{
public:
    enum Attributes
    {
        Scalars = 0,
        Vectors = 1,
        Normals = 2,
        TexCoords = 3,
        Tensors = 4,
        Any = 5
    };

    enum Flags
    {
        None = 0,
        Normalize = 1,
        RequireDouble = 2
    };

#if 0
    static coDoGeometry *vtk2Covise(const coObjInfo &info, vtkDataSet *vtk);
    static coDoGrid *vtkGrid2Covise(const coObjInfo &info, vtkDataSet *vtk);
    static coDoAbstractData *vtkData2Covise(const coObjInfo &info, vtkDataArray *varr, const coDoAbstractStructuredGrid *sgrid = NULL);
    static coDoAbstractData *vtkData2Covise(const coObjInfo &info, vtkDataSetAttributes *vtk, int attribute, const char *name = NULL, const coDoAbstractStructuredGrid *sgrid = NULL);
    static coDoAbstractData *vtkData2Covise(const coObjInfo &info, vtkDataSet *vtk, int attribute, const char *name = NULL, const coDoAbstractStructuredGrid *sgrid = NULL);
    static coDoPixelImage *vtkImage2Covise(const coObjInfo &info, vtkImageData *vtk);
#endif
};

#endif
