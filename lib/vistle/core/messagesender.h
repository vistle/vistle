#ifndef VISTLE_CORE_MESSAGESENDER_H
#define VISTLE_CORE_MESSAGESENDER_H

#include "export.h"
#include "message.h"
#include "envelope.h"

#include <vistle/util/buffer.h>

namespace vistle {

class V_COREEXPORT MessageSender {
public:
    virtual ~MessageSender();
    virtual bool sendMessage(const message::Envelope &payload) const = 0;
};

} // namespace vistle
#endif
