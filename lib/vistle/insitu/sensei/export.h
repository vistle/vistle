#ifndef VISTLE_SENSEI_EXPORT_H
#define VISTLE_SENSEI_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_sensei_EXPORTS)
#define V_SENSEIEXPORT V_EXPORT
#else
#define V_SENSEIEXPORT V_IMPORT
#endif

#if defined(vistle_sensei_EXPORTS)
#define V_SENSEI_TEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_SENSEI_TEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif

#endif
