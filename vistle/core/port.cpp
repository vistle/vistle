#include "port.h"

namespace vistle {

Port::Port(int m, const std::string & n, const Port::Type t, int f)
   : moduleID(m), name(n), type(t), m_flags(f) {

}

int Port::getModuleID() const {

   return moduleID;
}

const std::string & Port::getName() const {

   return name;
}

Port::Type Port::getType() const {

   return type;
}

int Port::flags() const {

   return m_flags;
}

ObjectList &Port::objects() {

   return m_objects;
}

const ObjectList &Port::objects() const {

   return m_objects;
}

const Port::PortSet &Port::connections() const {

   return m_connections;
}

void Port::setConnections(const PortSet &conn) {

   m_connections = conn;
}

bool Port::addConnection(Port *other) {

   return m_connections.insert(other).second;
}

Port *Port::removeConnection(const Port *other) {

   auto it = m_connections.find(const_cast<Port *>(other));
   if (it == m_connections.end())
      return NULL;

   Port *p = *it;
   m_connections.erase(it);
   return p;
}

bool Port::isConnected() const {

   return !m_connections.empty();
}

Port *Port::child(size_t idx, bool link) {

   assert(m_flags & Port::MULTI);
   if (!(m_flags & Port::MULTI))
      return NULL;

   if (m_children.size() > idx)
      return m_children[idx];

   const size_t first = m_children.size();
   for (size_t i=first; i<=idx; ++i) {
      m_children.push_back(new Port(getModuleID(), getName(), getType(), flags() & ~Port::MULTI));
   }

   if (link) {
      for (PortSet::iterator it = m_linkedPorts.begin(); it != m_linkedPorts.end(); ++it) {
         for (size_t i=first; i<=idx; ++i) {
            m_children[i]->link((*it)->child(i));
         }
      }
   }

   return m_children[idx];
}

bool Port::link(Port *other) {

   if (getType() == Port::INPUT) {
      assert(other->getType() == Port::OUTPUT);
      return false;
   }

   if (getType() == Port::OUTPUT) {
      assert(other->getType() == Port::INPUT);
      return false;
   }

   bool ok = true;
   if (m_linkedPorts.find(other) == m_linkedPorts.end()) {
      m_linkedPorts.insert(other);
   } else {
      ok = false;
   }

   if (other->m_linkedPorts.find(this) == other->m_linkedPorts.end()) {
      other->m_linkedPorts.insert(this);
   } else {
      ok = false;
   }

   return ok;
}

const Port::PortSet &Port::linkedPorts() const {

   return m_linkedPorts;
}

std::ostream &operator<<(std::ostream &s, const Port &port) {

    s << port.getModuleID() << ":" << port.getName() << "(" << port.getType() << ")";
    return s;
}

} // namespace vistle
