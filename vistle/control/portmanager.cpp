#include "portmanager.h"

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

PortManager::PortManager() {

}

Port * PortManager::addPort(const int moduleID, const std::string & name,
                          const Port::Type type) {

   std::map<std::string, Port *> *portMap = NULL;
   std::map<int, std::map<std::string, Port *> *>::iterator i =
      ports.find(moduleID);

   if (i == ports.end()) {
      portMap = new std::map<std::string, Port *>;
      ports[moduleID] = portMap;
   } else
      portMap = i->second;

   std::map<std::string, Port *>::iterator pi = portMap->find(name);

   if (pi == portMap->end()) {
      Port *port = new Port(moduleID, name, type);
      portMap->insert(std::pair<std::string, Port *>
                      (name, port));
      connections[port] = new std::vector<const Port *>;
      
      return port;
   } else
      return pi->second;
}

Port * PortManager::getPort(const int moduleID,
                            const std::string & name) const {

   std::map<int, std::map<std::string, Port *> *>::const_iterator i =
      ports.find(moduleID);

   if (i != ports.end()) {
      std::map<std::string, Port *>::iterator pi = i->second->find(name);
      if (pi != i->second->end())
         return pi->second;
   }

   return NULL;
}

void PortManager::addConnection(const Port * out, const Port * in) {

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

void PortManager::addConnection(const int a, const std::string & na,
                                const int b, const std::string & nb) {

   Port *portA = getPort(a, na);
   Port *portB = getPort(b, nb);

   if (!portA)
      portA = addPort(a, na, Port::OUTPUT);
   if (!portB)
      portB = addPort(b, nb, Port::INPUT);

   if (portA && portB) 
      addConnection(portA, portB);
}

const std::vector<const Port *> *
PortManager::getConnectionList(const Port * port) const {

   std::map<const Port *, std::vector<const Port *> *>::const_iterator i =
      connections.find(port);
   if (i != connections.end())
      return i->second;
   else
      return NULL;
}

const std::vector<const Port *> *
PortManager::getConnectionList(const int moduleID,
                               const std::string & name) const {

   const Port *port = getPort(moduleID, name);
   return getConnectionList(port);
}

std::vector<std::string> PortManager::getPortNames(const int moduleID, Port::Type type) const {

   std::vector<std::string> result;

   typedef std::map<std::string, Port *> PortMap;
   typedef std::map<int, PortMap *> ModulePortMap;

   ModulePortMap::const_iterator mports = ports.find(moduleID);
   if (mports == ports.end())
      return result;

   const PortMap &portmap = *mports->second;
   for(PortMap::const_iterator it = portmap.begin();
         it != portmap.end();
         ++it) {

      if (type == Port::ANY || it->second->getType() == type)
         result.push_back(it->first);
   }

   return result;
}

std::vector<std::string> PortManager::getInputPortNames(const int moduleID) const {

   return getPortNames(moduleID, Port::INPUT);
}

std::vector<std::string> PortManager::getOutputPortNames(const int moduleID) const {

   return getPortNames(moduleID, Port::OUTPUT);
}


} // namespace vistle
