#include "rfbext.h"


vistle::message::RemoteRenderMessage::RemoteRenderMessage(const vistle::RhrSubMessage &rhr, size_t payloadSize)
{
    m_payloadSize = payloadSize;

    size_t sz = 0;
    switch (rhr.type) {
    case rfbMatrices:
        sz = sizeof(matricesMsg);
        break;
    case rfbLights:
        sz = sizeof(lightsMsg);
        break;
    case rfbTile:
        sz = sizeof(tileMsg);
        break;
    case rfbBounds:
        sz = sizeof(boundsMsg);
        break;
    case rfbAnimation:
        sz = sizeof(animationMsg);
        break;
    case rfbVariant:
        sz = sizeof(variantMsg);
        break;
    }
    memcpy(m_rhr.data(), &rhr, sz);
}

const vistle::RhrSubMessage &vistle::message::RemoteRenderMessage::rhr() const
{
    return *reinterpret_cast<const rfbMsg *>(m_rhr.data());
}

vistle::RhrSubMessage &vistle::message::RemoteRenderMessage::rhr()
{
    return *reinterpret_cast<rfbMsg *>(m_rhr.data());
}
