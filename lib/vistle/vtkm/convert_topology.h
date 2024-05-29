#ifndef VISTLE_VTKM_CONVERT_CELLSET_H
#define VISTLE_VTKM_CONVERT_CELLSET_H

#include <vistle/core/object.h>
#include <vistle/core/scalar.h>
#include <vistle/core/triangles.h>
#include <vistle/core/vec.h>

#include <vtkm/cont/DataSet.h>

#include "export.h"
#include "convert.h"

namespace vistle {

Object::ptr vtkmGetTopology(const vtkm::cont::DataSet &dataset);
VtkmTransformStatus vtkmSetTopology(vtkm::cont::DataSet &vtkmDataset, vistle::Object::const_ptr grid);
} // namespace vistle

#endif
