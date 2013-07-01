#ifndef USERINTERFACE_EXPORT_H
#define USERINTERFACE_EXPORT_H

#include <util/export.h>

#if defined (vistle_ui_EXPORTS)
#define V_UIEXPORT V_EXPORT
#else
#define V_UIEXPORT V_IMPORT
#endif

#endif
