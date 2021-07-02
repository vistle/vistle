#include "InSituMessage.h"

#include <iostream>

#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/creation_tags.hpp>

using namespace vistle::insitu;
using namespace vistle::insitu::message;

InSituMessageType InSituMessageBase::type() const
{
    return m_type;
}

InSituMessageType Message::type() const
{
    return m_type;
}

Message Message::errorMessage()
{
    return Message{};
}

Message::Message(InSituMessageType type, vistle::buffer &&payload): m_type(type), m_payload(payload)
{}

Message::Message(): m_type(InSituMessageType::Invalid)
{}
