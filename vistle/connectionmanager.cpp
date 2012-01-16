#include "connectionmanager.h"

namespace vistle {

Port::Port(int m, const std::string &n, Port::Type t)
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

void Port::addObject(const std::string & name) {

   objects.push_back(name);
}

ConnectionManager::ConnectionManager() {

}

void ConnectionManager::addPort(const int moduleID, const std::string &name,
                                const Port::Type type) {

   std::map<std::string, Port *> *portMap = NULL;
   std::map<int, std::map<std::string, Port *> *>::iterator i =
      ports.find(moduleID);

   if (i == ports.end()) {
      portMap = new std::map<std::string, Port *>;
      ports[moduleID] = portMap;
   } else
      portMap = i->second;

   Port *port = new Port(moduleID, name, type);
   portMap->insert(std::pair<std::string, Port *>
                   (name, port));

   connections[port] = new std::vector<const Port *>;
}

Port *ConnectionManager::getPort(const int moduleID,
                                 const std::string name) const {

   std::map<int, std::map<std::string, Port *> *>::const_iterator i =
      ports.find(moduleID);

   if (i != ports.end()) {
      std::map<std::string, Port *>::iterator pi = i->second->find(name);
      if (pi != i->second->end())
         return pi->second;
   }

   return NULL;
}

void ConnectionManager::addConnection(const Port *out, const Port *in) {

   if (out->getType() == Port::OUTPUT && in->getType() == Port::INPUT) {

      std::map<const Port *, std::vector<const Port *> *>::iterator outi =
         connections.find(out);

      std::map<const Port *, std::vector<const Port *> *>::iterator ini =
         connections.find(in);

      if (outi != connections.end())
         outi->second->push_back(in);

      if (ini != connections.end())
         ini->second->push_back(in);
   }
}

void ConnectionManager::addConnection(const int a, const std::string na,
                                      const int b, const std::string nb) {

   std::map<int, std::map<std::string, Port *> *>::iterator ia =
      ports.find(a);
   std::map<int, std::map<std::string, Port *> *>::iterator ib =
      ports.find(b);

   if (ia != ports.end() && ib != ports.end()) {

      std::map<std::string, Port *>::iterator pia = ia->second->find(na);
      std::map<std::string, Port *>::iterator pib = ib->second->find(nb);
      if (pia != ia->second->end() && pib != ib->second->end())
         addConnection(pia->second, pib->second);
   }
}

const std::vector<const Port *> *
ConnectionManager::getConnectionList(const Port *port) const {

   std::map<const Port *, std::vector<const Port *> *>::const_iterator i =
      connections.find(port);
   if (i != connections.end())
      return i->second;
   else
      return NULL;
}

const std::vector<const Port *> *
ConnectionManager::getConnectionList(const int moduleID,
                                     const std::string &name) const {

   const Port *port = getPort(moduleID, name);
   return getConnectionList(port);
}


} // namespace vistle
