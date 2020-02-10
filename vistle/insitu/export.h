#ifndef VISTLE_INSITU_EXPORT_H
#define VISTLE_INSITU_EXPORT_H

#include <util/export.h>

#if defined (vistle_insitu_EXPORTS)
#define V_INSITUEXPORT V_EXPORT
#else
#define V_INSITUEXPORT V_IMPORT
#endif

#endif
