#include "portmanager.h"
#include "communicator.h"
#include <core/message.h>
#include <iostream>
#include <algorithm>

namespace vistle {

PortManager::PortManager() {

}

Port * PortManager::addPort(const int moduleID, const std::string & name,
                          const Port::Type type) {
   Port *port = new Port(moduleID, name, type);
   return addPort(port);
}

Port *PortManager::addPort(Port *port) {

   const int moduleID = port->getModuleID();
   PortMap *portMap = NULL;
   std::map<int, PortMap *>::iterator i = m_ports.find(moduleID);

   if (i == m_ports.end()) {
      portMap = new std::map<std::string, Port *>;
      m_ports[moduleID] = portMap;
   } else {
      portMap = i->second;
   }

   assert(portMap);

   PortMap::iterator pi = portMap->find(port->getName());

   if (pi == portMap->end()) {
      portMap->insert(std::make_pair(port->getName(), port));
      m_connections[port] = new std::vector<const Port *>;
      
      return port;
   } else {
      if (pi->second->getType() == Port::ANY) {
         port->setConnections(pi->second->connections());
         delete pi->second;
         pi->second = port;
         return port;
      } else {
         delete port;
         return pi->second;
      }
   }
}

Port * PortManager::getPort(const int moduleID,
                            const std::string & name) const {

   std::map<int, std::map<std::string, Port *> *>::const_iterator i =
      m_ports.find(moduleID);

   if (i != m_ports.end()) {
      std::map<std::string, Port *>::iterator pi = i->second->find(name);
      if (pi != i->second->end())
         return pi->second;
   }

   std::string::size_type p = name.find('[');
   if (p != std::string::npos) {
      Port *parent = getPort(moduleID, name.substr(0, p-1));
      if (parent && (parent->flags() & Port::MULTI)) {
         std::stringstream idxstr(name.substr(p+1));
         size_t idx=0;
         idxstr >> idx;
         Port *port = parent->child(idx);
         Communicator::the().sendMessage(moduleID, message::CreatePort(port));
         return port;
      }
   }

   return NULL;
}

bool PortManager::addConnection(const Port *from, const Port *to) {

   if (from->getType() == Port::OUTPUT && to->getType() == Port::INPUT) {

      std::map<const Port *, ConnectionList *>::iterator outi = m_connections.find(from);
      if (outi != m_connections.end())
         outi->second->push_back(to);

      std::map<const Port *, ConnectionList *>::iterator ini = m_connections.find(to);
      if (ini != m_connections.end())
         ini->second->push_back(from);

      return true;
   }

   if (from->getType() == Port::PARAMETER && to->getType() == Port::PARAMETER) {

      std::map<const Port *, ConnectionList *>::iterator outi = m_connections.find(from);
      if (outi != m_connections.end())
         outi->second->push_back(to);

      std::map<const Port *, ConnectionList *>::iterator ini = m_connections.find(to);
      if (ini != m_connections.end())
         ini->second->push_back(from);

      return true;
   }

   return false;
}

bool PortManager::addConnection(const int a, const std::string & na,
                                const int b, const std::string & nb) {

   // FIXME: we add ports, as they might not yet have been created by the module
   Port *portA = getPort(a, na);
   if (!portA) {
      portA = addPort(a, na, Port::ANY);
   }
   Port *portB = getPort(b, nb);
   if (!portB) {
      portB = addPort(b, nb, Port::ANY);
   }

   return addConnection(portA, portB);
}

bool PortManager::removeConnection(const Port *from, const Port *to) {

   bool ok = false;
   if ((from->getType() == Port::OUTPUT && to->getType() == Port::INPUT)
      || (from->getType() == Port::PARAMETER && to->getType() == Port::PARAMETER)) {

      ok = true;
      std::map<const Port *, std::vector<const Port *> *>::iterator outi =
         m_connections.find(from);
      if (outi != m_connections.end()) {
         ConnectionList &cl = *outi->second;
         ConnectionList::iterator it = std::find(cl.begin(), cl.end(), to);
         if (it != cl.end())
            cl.erase(it);
         else
            ok = false;
      } else {
         ok = false;
      }

      std::map<const Port *, std::vector<const Port *> *>::iterator ini =
         m_connections.find(to);
      if (ini != m_connections.end()) {
         ConnectionList &cl = *ini->second;
         ConnectionList::iterator it = std::find(cl.begin(), cl.end(), from);
         if (it != cl.end())
            cl.erase(it);
         else
            ok = false;
      } else {
         ok = false;
      }
   } else {
      ok = false;
   }

   return ok;
}

bool PortManager::removeConnection(const int a, const std::string & na,
      const int b, const std::string & nb) {

   Port *from = getPort(a, na);
   Port *to = getPort(b, nb);

   if (!from || !to)
      return false;

   return removeConnection(from, to);
}

//! remove all connections to and from ports to a module
void PortManager::removeConnections(const int moduleID) {

   typedef std::map<std::string, Port *> PortMap;
   typedef std::map<int, PortMap *> ModulePortMap;

   ModulePortMap::const_iterator mports = m_ports.find(moduleID);
   if (mports == m_ports.end())
      return;

   const PortMap &portmap = *mports->second;
   for(PortMap::const_iterator it = portmap.begin();
         it != portmap.end();
         ++it) {

      Port *port = it->second;
         std::map<const Port *, std::vector<const Port *> *>::iterator connlist = m_connections.find(port);
         if (connlist != m_connections.end()) {
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
      m_connections.find(port);
   if (i != m_connections.end())
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

   ModulePortMap::const_iterator mports = m_ports.find(moduleID);
   if (mports == m_ports.end())
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
