#ifndef VISTLE_MINISIM_MINISIM_EXPORT_H
#define VISTLE_MINISIM_MINISIM_EXPORT_H

#include <vistle/util/export.h>

#if defined(miniSim_EXPORTS)
#define MINI_SIM_EXPORT V_EXPORT
#else
#define MINI_SIM_EXPORT V_IMPORT
#endif

#endif
