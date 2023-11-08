#ifndef VISTLE_VTKM_UTILS_H
#define VISTLE_VTKM_UTILS_H

#include <vistle/core/object.h>
#include <vistle/core/scalar.h>
#include <vistle/core/triangles.h>
#include <vistle/core/vec.h>

#include <vtkm/cont/DataSet.h>

#include <vector>

enum VTKM_TRANSFORM_STATUS { SUCCESS = 0, UNSUPPORTED_GRID_TYPE = 1, UNSUPPORTED_CELL_TYPE = 2 };

// transform a vistle dataset (grid and scalar data field) into a vtkm dataset
// so vtkm filters can be applied to it
VTKM_TRANSFORM_STATUS vistleToVtkmDataSet(vistle::Object::const_ptr grid,
                                          std::shared_ptr<const vistle::Vec<vistle::Scalar, 1U>> scalarField,
                                          vtkm::cont::DataSet &vtkmDataSet, bool useArrayHandles);

// transform a vtkm isosurface dataset into a vistle Triangles object, so it can
// be rendered in COVER
vistle::Triangles::ptr vtkmIsosurfaceToVistleTriangles(vtkm::cont::DataSet &isosurface);

#endif // VISTLE_VTKM_UTILS_H