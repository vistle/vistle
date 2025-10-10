#ifndef VISTLE_MODULE_MODULE_IMPL_H
#define VISTLE_MODULE_MODULE_IMPL_H

#include <vistle/core/message.h>
#include <vistle/core/messages.h>
#include <cassert>

namespace vistle {

template<class Type>
typename Type::const_ptr BlockTask::accept(const Port *port)
{
    auto it = m_input.find(port);
    if (it == m_input.end())
        return nullptr;
    auto obj = it->second;
    if (!obj) {
        std::stringstream str;
        str << "did not receive valid object at " << port->getName() << std::endl;
        m_module->sendWarning(str.str());
        return nullptr;
    }
    assert(obj->check(std::cerr));
    auto ret = Type::as(obj);
    if (ret)
        m_input.erase(it);
    return ret;
}
template<class Type>
typename Type::const_ptr BlockTask::accept(const std::string &port)
{
    auto it = m_portsByString.find(port);
    if (it == m_portsByString.end())
        return nullptr;
    return accept<Type>(it->second);
}

template<class Type>
typename Type::const_ptr BlockTask::expect(const Port *port)
{
    if (!port) {
        std::stringstream str;
        str << "invalid port" << std::endl;
        m_module->sendError(str.str());
        return nullptr;
    }
    if (!m_module->isConnected(*port)) {
        std::stringstream str;
        str << "port " << port->getName() << " is not connected" << std::endl;
        m_module->sendError(str.str());
        return nullptr;
    }
    auto it = m_input.find(port);
    assert(it != m_input.end());
    if (it == m_input.end()) {
        if (m_module->schedulingPolicy() == message::SchedulingPolicy::Single) {
            std::stringstream str;
            str << "no object available at " << port->getName() << ", but " << Type::typeName() << " is required"
                << std::endl;
            m_module->sendError(str.str());
        }
        return nullptr;
    }
    auto obj = it->second;
    m_input.erase(it);
    if (!obj) {
        std::stringstream str;
        str << "did not receive object at " << port->getName() << std::endl;
        m_module->sendWarning(str.str());
        return nullptr;
    }
    assert(obj->check(std::cerr));
    return Type::as(obj);
}
template<class Type>
typename Type::const_ptr BlockTask::expect(const std::string &port)
{
    auto it = m_portsByString.find(port);
    assert(it != m_portsByString.end());
    return expect<Type>(it->second);
}

template<class Type>
typename Type::const_ptr Module::accept(Port *port)
{
    Object::const_ptr obj;
    if (port->objects().empty()) {
        return nullptr;
    }
    obj = port->objects().front();
    if (!obj) {
        port->objects().pop_front();
        std::stringstream str;
        str << "did not receive valid object at " << port->getName() << std::endl;
        sendWarning(str.str());
        return nullptr;
    }
    assert(obj->check(std::cerr));
    typename Type::const_ptr ret = Type::as(obj);
    if (ret)
        port->objects().pop_front();
    return ret;
}
template<class Type>
typename Type::const_ptr Module::accept(const std::string &port)
{
    Port *p = findInputPort(port);
    assert(p);
    return accept<Type>(p);
}

template<class Type>
typename Type::const_ptr Module::expect(Port *port)
{
    if (!port) {
        std::stringstream str;
        str << "invalid port" << std::endl;
        sendError(str.str());
        return nullptr;
    }
    if (!isConnected(*port)) {
        std::stringstream str;
        str << "port " << port->getName() << " is not connected" << std::endl;
        sendError(str.str());
        return nullptr;
    }
    if (port->objects().empty()) {
        if (schedulingPolicy() == message::SchedulingPolicy::Single) {
            std::stringstream str;
            str << "no object available at " << port->getName() << ", but " << Type::typeName() << " is required"
                << std::endl;
            sendError(str.str());
        }
        return nullptr;
    }
    Object::const_ptr obj = port->objects().front();
    typename Type::const_ptr ret = Type::as(obj);
    port->objects().pop_front();
    if (!obj) {
        std::stringstream str;
        str << "did not receive valid object at " << port->getName() << ", but " << Type::typeName() << " is required"
            << std::endl;
        sendError(str.str());
        return ret;
    }
    assert(obj->check(std::cerr));
    if (!ret) {
        std::stringstream str;
        str << "received " << Object::toString(obj->getType()) << " at " << port->getName() << ", but "
            << Type::typeName() << " is required" << std::endl;
        sendError(str.str());
    }
    return ret;
}
template<class Type>
typename Type::const_ptr Module::expect(const std::string &port)
{
    Port *p = findInputPort(port);
    assert(p);
    return expect<Type>(p);
}

template<class Payload>
bool Module::sendMessageWithPayload(message::Message &message, Payload &payload) const
{
    auto pl = addPayload(message, payload);
    return this->sendMessage(message, &pl);
}


} // namespace vistle
#endif
