#ifndef VISTLE_CORE_ENVELOPE_H
#define VISTLE_CORE_ENVELOPE_H

#include "export.h"
#include <vector>
#include "message.h"
#include <vistle/util/buffer.h>
namespace vistle {

namespace message {
class V_COREEXPORT Envelope {
public:
    Envelope() = default; //set buffer() manually and use updatePayload() to eventually get shm/internal payload
    Envelope(const message::Message &msg); // MessagePayload without payload
    Envelope(const message::Buffer &msg); // overload for Buffer so that it can be copied with its full payload

    virtual ~Envelope() = default;
    virtual std::unique_ptr<Envelope> clone() const;

    message::Buffer &message();
    const message::Buffer &message() const;

    const char *payloadData() const;
    size_t payloadSize() const;
    buffer copyPayload() const;


    size_t headerSize() const; // message + eventual internal payload size
    size_t externalPayloadSize() const;
    bool hasInternalPayload() const;

    // use to set m_internalPayload after Buffer is read from a stream
    void updateMessage();

    template<typename SomeMessage>
    SomeMessage &as()
    {
        return m_message.as<SomeMessage>();
    }

    template<typename SomeMessage>
    const SomeMessage &as() const
    {
        return m_message.as<SomeMessage>();
    }

    // save a copy if we already have a payload as buffer
    // todo: change the archive functions so that they can work on other arrays
    template<typename Payload>
    Payload deserializePayload() const
    {
        assert(payloadSize() > 0);
        return message::getPayload<Payload>({payloadData(), payloadData() + payloadSize()});
    }

protected:
    Envelope(const Envelope &other);
    Envelope &operator=(const Envelope &other);
    Envelope(const message::Message &msg, const char *payload, size_t payloadSize);

    virtual const char *getExternalPayload() const;

    // keep m_buffer first
    message::Buffer m_message;
    const char *m_internalPayload = nullptr;
};

class V_COREEXPORT BufferEnvelope: public Envelope {
public:
    using Envelope::Envelope;
    BufferEnvelope(const message::Message &msg, const vistle::buffer &payload); // copy payload to buffer
    BufferEnvelope(const message::Message &msg,
                   std::shared_ptr<vistle::buffer> payload); // use existing buffer as payload
    std::unique_ptr<Envelope> clone() const override;
    const std::shared_ptr<vistle::buffer> &bufferPayload() const;
    std::shared_ptr<vistle::buffer> &bufferPayload();

private:
    const char *getExternalPayload() const override;
    std::shared_ptr<vistle::buffer> m_payload;
};

} // namespace message
} // namespace vistle
#endif
