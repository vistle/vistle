#ifndef CONTROL_EXPORT_H
#define CONTROL_EXPORT_H

#include <util/export.h>

#if defined (vistle_control_EXPORTS)
#define CONTROLEXPORT VEXPORT
#else
#define CONTROLEXPORT VIMPORT
#endif

#endif
