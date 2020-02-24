#ifndef VISTLE_COVER_MESSAGE_H
#define VISTLE_COVER_MESSAGE_H

#include <core/messagepayload.h>
#include <core/message.h>

struct VistleMessage {
    VistleMessage(const vistle::message::Message &msg, const vistle::MessagePayload &payload)
    : buf(msg)
    , payload(payload)
    {}

    vistle::message::Buffer buf;
    vistle::MessagePayload payload;
};
#endif
