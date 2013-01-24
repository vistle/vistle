#ifndef VISTLE_EXPORT_H
#define VISTLE_EXPORT_H

#include <util/export.h>

#if defined (vistle_core_EXPORTS)
#define VCEXPORT VEXPORT
#else
#define VCEXPORT VIMPORT
#endif

#endif
