#ifndef VISTLE_MESSAGESENDER_H
#define VISTLE_MESSAGESENDER_H

#include "export.h"
#include "message.h"

namespace vistle {

class  V_COREEXPORT MessageSender {
 public:
    virtual bool sendMessage(const message::Message &message) const = 0;
};

}
#endif
