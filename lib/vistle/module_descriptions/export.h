#ifndef VISTLE_MODULE_DESCRIPTIONS_EXPORT_H
#define VISTLE_MODULE_DESCRIPTIONS_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_module_descriptions_EXPORTS)
#define V_MODULEDESCRIPTIONSEXPORT V_EXPORT
#else
#define V_MODULEDESCRIPTIONSEXPORT V_IMPORT
#endif

#endif
