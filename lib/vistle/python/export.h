#ifndef VISTLE_PYTHON_EXPORT_H
#define VISTLE_PYTHON_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_python_EXPORTS)
#define V_PYEXPORT V_EXPORT
#else
#define V_PYEXPORT V_IMPORT
#endif

#endif
