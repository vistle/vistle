#ifndef VTKM_TRANSFORM_ENUMS_H
#define VTKM_TRANSFORM_ENUMS_H

// x and y coordinates are swapped in VTKm (compared to vistle)
enum AxesSwap { xId = 2, yId = 1, zId = 0 };

enum VtkmTransformStatus { SUCCESS = 0, UNSUPPORTED_GRID_TYPE = 1, UNSUPPORTED_CELL_TYPE = 2, UNSUPPORTED_FIELD_TYPE };

#endif // VTKM_TRANSFORM_ENUMS_H
