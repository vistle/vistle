#include "port.h"
#include <cassert>

namespace vistle {

Port::Port(int m, const std::string &n, const Port::Type t, unsigned f)
: moduleID(m), name(n), type(t), m_flags(f), m_parent(nullptr)
{}

void Port::setDescription(const std::string &desc)
{
    description = desc;
}

const std::string &Port::getDescription() const
{
    return description;
}

int Port::getModuleID() const
{
    return moduleID;
}

const std::string &Port::getName() const
{
    return name;
}

Port::Type Port::getType() const
{
    return type;
}

unsigned Port::flags() const
{
    return m_flags;
}

ObjectList &Port::objects()
{
    return m_objects;
}

const ObjectList &Port::objects() const
{
    return m_objects;
}

const Port::ConstPortSet &Port::connections() const
{
    return m_connections;
}

void Port::setConnections(const ConstPortSet &conn)
{
    m_connections = conn;
}

bool Port::addConnection(Port *other)
{
    return m_connections.insert(other).second;
}

const Port *Port::removeConnection(const Port &other)
{
    auto it = m_connections.find(&other);
    if (it == m_connections.end()) {
        return nullptr;
    }

    const Port *p = *it;
    m_connections.erase(it);
    return p;
}

bool Port::isConnected() const
{
    return !m_connections.empty();
}

Port *Port::child(size_t idx, bool link)
{
    assert(m_flags & Port::MULTI);
    if (!(m_flags & Port::MULTI))
        return NULL;

    if (m_children.size() > idx)
        return m_children[idx];

    const size_t first = m_children.size();
    if (idx > first)
        m_children.reserve(idx - first);
    for (size_t i = first; i <= idx; ++i) {
        m_children.push_back(new Port(getModuleID(), getName(), getType(), flags() & ~Port::MULTI));
    }

    if (link) {
        for (auto it = m_linkedPorts.begin(); it != m_linkedPorts.end(); ++it) {
            for (size_t i = first; i <= idx; ++i) {
                m_children[i]->link((*it)->child(i));
            }
        }
    }

    return m_children[idx];
}

bool Port::link(Port *linked)
{
    if (getType() == Port::INPUT) {
        assert(linked->getType() == Port::OUTPUT);
        return false;
    }

    if (getType() == Port::OUTPUT) {
        assert(linked->getType() == Port::INPUT);
        return false;
    }

    bool ok = true;
    if (m_linkedPorts.find(linked) == m_linkedPorts.end()) {
        m_linkedPorts.insert(linked);
    } else {
        ok = false;
    }

    if (linked->m_linkedPorts.find(this) == linked->m_linkedPorts.end()) {
        linked->m_linkedPorts.insert(this);
    } else {
        ok = false;
    }

    return ok;
}

const Port::PortSet &Port::linkedPorts() const
{
    return m_linkedPorts;
}

std::ostream &operator<<(std::ostream &s, const Port &port)
{
    s << port.getModuleID() << ":" << port.getName() << "(" << port.getType() << ")";
    return s;
}

} // namespace vistle
