#ifndef VISTLE_MESSAGESENDER_H
#define VISTLE_MESSAGESENDER_H

#include "export.h"
#include "message.h"

#include <util/buffer.h>

namespace vistle {

class V_COREEXPORT MessageSender {
public:
    virtual ~MessageSender();
    virtual bool sendMessage(const message::Message &msg, const buffer *payload=nullptr) const = 0;
};

}
#endif
