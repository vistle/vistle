#ifndef VISTLE_PYTHON_MODULE_EXPORT_H
#define VISTLE_PYTHON_MODULE_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_pythonmodule_EXPORTS)
#define V_PYMODEXPORT V_EXPORT
#else
#define V_PYMODEXPORT V_IMPORT
#endif

#endif
