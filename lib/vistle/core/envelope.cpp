#include "envelope.h"
#include "message.h"

namespace vistle {
namespace message {

Envelope::Envelope(const message::Message &msg): m_message(msg)
{
    assert(msg.payloadSize() == 0);
}

Envelope::Envelope(const message::Message &msg, const char *payload, size_t payloadSize): m_message(msg)
{
    m_internalPayload = m_message.addPayload(payload, payloadSize);
    m_message.setPayloadSize(payloadSize);
}

Envelope::Envelope(const message::Buffer &msg): m_message(msg), m_internalPayload(m_message.getPayload())
{}

std::unique_ptr<Envelope> Envelope::clone() const
{
    return std::make_unique<Envelope>(this->m_message);
}

Envelope::Envelope(const Envelope &other): m_message(other.m_message)
{
    m_internalPayload = m_message.getPayload();
}

Envelope &Envelope::operator=(const Envelope &other)
{
    if (this != &other) {
        m_message = other.m_message;
        m_internalPayload = m_message.getPayload();
    }
    return *this;
}

void Envelope::updateMessage()
{
    if (m_message.payloadSize() == 0)
        return;
    m_internalPayload = m_message.getPayload();
}

buffer Envelope::copyPayload() const
{
    if (payloadSize() == 0) {
        return buffer();
    }
    return buffer(payloadData(), payloadData() + payloadSize());
}

const char *Envelope::payloadData() const
{
    if (m_message.payloadSize() == 0) {
        return nullptr;
    }
    assert(m_internalPayload || getExternalPayload());

    return m_internalPayload ? m_internalPayload : getExternalPayload();
}

size_t Envelope::headerSize() const
{
    return m_message.size() + (m_internalPayload ? m_message.payloadSize() : 0);
}

size_t Envelope::externalPayloadSize() const
{
    return m_internalPayload ? 0 : payloadSize();
}

message::Buffer &Envelope::message()
{
    return m_message;
}

const message::Buffer &Envelope::message() const
{
    return m_message;
}

bool Envelope::hasInternalPayload() const
{
    return m_internalPayload;
}

size_t Envelope::payloadSize() const
{
    assert(m_message.payloadSize() == 0 || m_internalPayload || getExternalPayload());
    return m_message.payloadSize();
}

const char *Envelope::getExternalPayload() const
{
    return m_internalPayload;
}


BufferEnvelope::BufferEnvelope(const message::Message &msg, const vistle::buffer &payload)
: Envelope(msg, payload.data(), payload.size())
{
    if (!m_internalPayload) {
        m_payload = std::make_shared<vistle::buffer>(payload);
        m_message.setPayloadSize(payload.size());
    }
}

BufferEnvelope::BufferEnvelope(const message::Message &msg, std::shared_ptr<vistle::buffer> payload)
: Envelope(msg, payload->data(), payload->size())
{
    if (!m_internalPayload) {
        m_payload = payload;
        m_message.setPayloadSize(payload->size());
    }
}

std::unique_ptr<Envelope> BufferEnvelope::clone() const
{
    return std::make_unique<BufferEnvelope>(*this);
}

const char *BufferEnvelope::getExternalPayload() const
{
    return m_payload ? m_payload->data() : nullptr;
}

const std::shared_ptr<vistle::buffer> &BufferEnvelope::bufferPayload() const
{
    return m_payload;
}

std::shared_ptr<vistle::buffer> &BufferEnvelope::bufferPayload()
{
    return m_payload;
}

} // namespace message
} // namespace vistle
