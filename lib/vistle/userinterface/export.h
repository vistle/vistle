#ifndef USERINTERFACE_EXPORT_H
#define USERINTERFACE_EXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_userinterface_EXPORTS)
#define V_UIEXPORT V_EXPORT
#else
#define V_UIEXPORT V_IMPORT
#endif

#endif
