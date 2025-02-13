#ifndef VISTLE_VTK_EXPORT_H
#define VISTLE_VTK_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_vtk_EXPORTS)
#define V_VTK_EXPORT V_EXPORT
#else
#define V_VTK_EXPORT V_IMPORT
#endif
#endif
