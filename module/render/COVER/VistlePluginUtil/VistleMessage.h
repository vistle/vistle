#ifndef VISTLE_COVER_VISTLEPLUGINUTIL_VISTLEMESSAGE_H
#define VISTLE_COVER_VISTLEPLUGINUTIL_VISTLEMESSAGE_H

#include <vistle/core/messagepayload.h>
#include <vistle/core/message.h>

struct VistleMessage {
    VistleMessage(const vistle::message::Message &msg, const vistle::MessagePayload &payload)
    : buf(msg), payload(payload)
    {}

    explicit VistleMessage(const vistle::message::Message &msg): buf(msg) {}

    vistle::message::Buffer buf;
    vistle::MessagePayload payload;
};
#endif
