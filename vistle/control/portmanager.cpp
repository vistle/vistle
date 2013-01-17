#include "portmanager.h"
#include <iostream>
#include <algorithm>

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

void PortManager::addConnection(const Port *from, const Port *to) {

   if (from->getType() == Port::OUTPUT && to->getType() == Port::INPUT) {

      std::map<const Port *, ConnectionList *>::iterator outi =
         connections.find(from);
      if (outi != connections.end())
         outi->second->push_back(to);

      std::map<const Port *, ConnectionList *>::iterator ini =
         connections.find(to);
      if (ini != connections.end())
         ini->second->push_back(from);

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

void PortManager::removeConnection(const Port *from, const Port *to) {

   if (from->getType() == Port::OUTPUT && to->getType() == Port::INPUT) {

      std::map<const Port *, std::vector<const Port *> *>::iterator outi =
         connections.find(from);
      if (outi != connections.end()) {
         ConnectionList &cl = *outi->second;
         ConnectionList::iterator it = std::find(cl.begin(), cl.end(), to);
         if (it != cl.end())
            cl.erase(it);
      }

      std::map<const Port *, std::vector<const Port *> *>::iterator ini =
         connections.find(to);
      if (ini != connections.end()) {
         ConnectionList &cl = *ini->second;
         ConnectionList::iterator it = std::find(cl.begin(), cl.end(), from);
         if (it != cl.end())
            cl.erase(it);
      }
   }
}

void PortManager::removeConnection(const int a, const std::string & na,
      const int b, const std::string & nb) {

   Port *from = getPort(a, na);
   Port *to = getPort(b, nb);

   if (!from || !to)
      return;

   removeConnection(from, to);
}

//! remove all connections to and from ports to a module
void PortManager::removeConnections(const int moduleID) {

   typedef std::map<std::string, Port *> PortMap;
   typedef std::map<int, PortMap *> ModulePortMap;

   ModulePortMap::const_iterator mports = ports.find(moduleID);
   if (mports == ports.end())
      return;

   const PortMap &portmap = *mports->second;
   for(PortMap::const_iterator it = portmap.begin();
         it != portmap.end();
         ++it) {

      Port *port = it->second;
         std::map<const Port *, std::vector<const Port *> *>::iterator connlist = connections.find(port);
         if (connlist != connections.end()) {
         ConnectionList &cl = *connlist->second;
         
         while (!cl.empty()) {
            size_t oldsize = cl.size();
            const Port *other = cl.back();
            removeConnection(port, other);
            removeConnection(other, port);
            if (cl.size() == oldsize) {
               std::cerr << "failed to remove all connections for module " << moduleID << ", still left: " << cl.size() << std::endl;
               for (int i=0; i<cl.size(); ++i) {
                  std::cerr << "   " << port->getModuleID() << ":" << port->getName() << " <--> " << other->getModuleID() << ":" << other->getName() << std::endl;
               }
               break;
            }
         }
      }
   }
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
