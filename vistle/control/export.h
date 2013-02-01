#ifndef CONTROL_EXPORT_H
#define CONTROL_EXPORT_H

#include <util/export.h>

#if defined (vistle_control_EXPORTS)
#define V_CONTROLEXPORT V_EXPORT
#else
#define V_CONTROLEXPORT V_IMPORT
#endif

#endif
