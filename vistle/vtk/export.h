#ifndef VISTLE_VTK_TO_VISTLE_EXPORT_H
#define VISTLE_VTK_TO_VISTLE_EXPORT_H

#include <vistle/util/export.h>
#ifdef SENSEI
#if defined(vistle_sensei_vtk_EXPORTS)
#define V_VTK_EXPORT V_EXPORT
#else
#define V_VTK_EXPORT V_IMPORT
#endif
#else
#if defined(vistle_vtk_EXPORTS)
#define V_VTK_EXPORT V_EXPORT
#else
#define V_VTK_EXPORT V_IMPORT
#endif
#endif

#endif
