#ifndef VISTLE_SENSEI_EXPORT_H
#define VISTLE_SENSEI_EXPORT_H

#include <util/export.h>

#if defined (vistle_sensei_EXPORTS)
#define V_SENSEIEXPORT V_EXPORT
#else
#define V_SENSEIEXPORT V_IMPORT
#endif

#endif
