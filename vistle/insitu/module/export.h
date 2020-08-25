#ifndef VISTLE_INSITU_MODULE_EXPORT_H
#define VISTLE_INSITU_MODULE_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_insitu_module_EXPORTS)
#define V_INSITUMODULEEXPORT V_EXPORT
#else
#define V_INSITUMODULEEXPORT V_IMPORT
#endif

#endif
