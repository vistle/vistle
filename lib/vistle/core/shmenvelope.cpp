#include "shmenvelope.h"

using namespace vistle;

ShmEnvelope::ShmEnvelope(const message::Message &msg, const vistle::buffer &payload)
: Envelope(msg, payload.data(), payload.size())
{
    if (!m_internalPayload) {
        m_payload = MessagePayload(payload.data(), payload.size());
        m_message.setPayloadName(m_payload.name());
        m_message.setPayloadSize(payload.size());
    }
}

ShmEnvelope::ShmEnvelope(const message::Message &msg, const MessagePayload &payload)
: Envelope(msg, payload->data(), payload->size())
{
    if (!m_internalPayload && payload->size() > 0) {
        m_payload = payload;
        m_message.setPayloadName(m_payload.name());
        m_message.setPayloadSize(payload->size());
    }
}

std::unique_ptr<message::Envelope> ShmEnvelope::clone() const
{
    return std::make_unique<ShmEnvelope>(*this);
}

void ShmEnvelope::ref() const
{
    if (m_payload) {
        const_cast<MessagePayload &>(m_payload).ref();
    }
}

void ShmEnvelope::unref() const
{
    if (m_payload) {
        const_cast<MessagePayload &>(m_payload).unref();
    }
}

const char *ShmEnvelope::getExternalPayload() const
{
    return m_payload ? m_payload->data() : nullptr;
}

void ShmEnvelope::getPayloadFromHeader()
{
    assert(!m_payload);
    m_payload = Shm::the().getArrayFromName<char>(m_message.payloadName());
    assert(m_payload);
}

const MessagePayload &ShmEnvelope::shmPayload() const
{
    return m_payload;
}

MessagePayload &ShmEnvelope::shmPayload()
{
    return m_payload;
}
