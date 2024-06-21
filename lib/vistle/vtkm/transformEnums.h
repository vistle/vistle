#ifndef VTKM_TRANSFORM_ENUMS_H
#define VTKM_TRANSFORM_ENUMS_H

// TODO: remove this (it is, e.g., not used in GetArrayContents anymore)
enum AxesSwap { xId = 0, yId = 1, zId = 2 };

enum VtkmTransformStatus { SUCCESS = 0, UNSUPPORTED_GRID_TYPE = 1, UNSUPPORTED_CELL_TYPE = 2, UNSUPPORTED_FIELD_TYPE };

#endif // VTKM_TRANSFORM_ENUMS_H
