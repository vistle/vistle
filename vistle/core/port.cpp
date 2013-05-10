#include "port.h"

namespace vistle {

Port::Port(int m, const std::string & n, Port::Type t)
   : moduleID(m), name(n), type(t) {

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

ObjectList &Port::objects() {

   return m_objects;
}

Port::PortSet &Port::connections() {

   return m_connections;
}

bool Port::isConnected() const {

   return !m_connections.empty();
}

} // namespace vistle
