#ifndef V_INSITUMESSAGEEXPORT_H
#define V_INSITUMESSAGEEXPORT_H

#include <vistle/util/export.h>

#if defined(vistle_insitu_message_EXPORTS)
#define V_INSITUMESSAGEEXPORT V_EXPORT
#else
#define V_INSITUMESSAGEEXPORT V_IMPORT
#endif

#if defined(vistle_insitu_message_EXPORTS)
#define V_INSITU_MESSAGE_TEMPLATE_EXPORT V_TEMPLATE_EXPORT
#else
#define V_INSITU_MESSAGE_TEMPLATE_EXPORT V_TEMPLATE_IMPORT
#endif

#endif
