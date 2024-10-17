#ifndef VISTLE_VTKM_EXPORT_H
#define VISTLE_VTKM_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_vtkm_EXPORTS)
#define V_VTKM_EXPORT V_EXPORT
#else
#define V_VTKM_EXPORT V_IMPORT
#endif

#endif
