#ifndef VISTLE_CORE_SHMENVELOPE_H
#define VISTLE_CORE_SHMENVELOPE_H

#include "envelope.h"
#include "export.h"
#include "message.h"
#include "messagepayload.h"
namespace vistle {
class V_COREEXPORT ShmEnvelope: public message::Envelope {
public:
    using Envelope::Envelope;
    ShmEnvelope(const message::Message &msg, const vistle::buffer &payload); // copy payload to shm
    ShmEnvelope(const message::Message &msg, const MessagePayload &payload); // use existing buffer as payload
    std::unique_ptr<Envelope> clone() const override;

    // increase reference count of payload in shared memory
    // mandatory before sending for each reciepient, otherwise the payload will be deleted when it goes out of scope
    void ref() const;
    // decrease when receiving a message with a payload in shared memory, otherwise the payload will never be deleted
    void unref() const;
    void constructPayloadfromShm();
    const MessagePayload &shmPayload() const;
    MessagePayload &shmPayload();

private:
    const char *getExternalPayload() const override;
    MessagePayload m_payload;
};
} // namespace vistle

#endif
