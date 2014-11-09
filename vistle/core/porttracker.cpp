#include "porttracker.h"
#include "statetracker.h"
#include "parameter.h"
#include <core/message.h>
#include <iostream>
#include <algorithm>

namespace vistle {

PortTracker::PortTracker()
{
}

PortTracker::~PortTracker() {
}

Port * PortTracker::addPort(const int moduleID, const std::string & name,
                          const Port::Type type) {
   Port *port = new Port(moduleID, name, type);
   return addPort(port);
}

Port *PortTracker::addPort(Port *port) {

   const int moduleID = port->getModuleID();

   PortMap *portMap = NULL;
   {
      std::map<int, PortMap *>::iterator i = m_ports.find(moduleID);
      if (i == m_ports.end()) {
         portMap = new PortMap;
         m_ports[moduleID] = portMap;
      } else {
         portMap = i->second;
      }
      assert(portMap);
   }

   PortOrder *portOrder = NULL;
   {
      std::map<int, PortOrder *>::iterator i = m_portOrders.find(moduleID);
      if (i == m_portOrders.end()) {
         portOrder = new PortOrder;
         m_portOrders[moduleID] = portOrder;
      } else {
         portOrder = i->second;
      }
      assert(portOrder);
   }

   PortMap::iterator pi = portMap->find(port->getName());

   if (pi == portMap->end()) {
      portMap->insert(std::make_pair(port->getName(), port));

      int paramNum = 0;
      const auto &rit = portOrder->rbegin();
      if (rit != portOrder->rend())
         paramNum = rit->first+1;
      (*portOrder)[paramNum] = port->getName();
      
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

Port * PortTracker::getPort(const int moduleID,
                            const std::string & name) const {

   std::map<int, PortMap *>::const_iterator i =
      m_ports.find(moduleID);

   if (i != m_ports.end()) {
      PortMap::iterator pi = i->second->find(name);
      if (pi != i->second->end())
         return pi->second;
   }

#if 0
   std::string::size_type p = name.find('[');
   if (p != std::string::npos) {
      Port *parent = getPort(moduleID, name.substr(0, p-1));
      if (parent && (parent->flags() & Port::MULTI)) {
         std::stringstream idxstr(name.substr(p+1));
         size_t idx=0;
         idxstr >> idx;
         Port *port = parent->child(idx);
         m_clusterManager->sendMessage(moduleID, message::CreatePort(port));
         return port;
      }
   }
#endif

   return NULL;
}

Port *PortTracker::getPort(const Port *p) const {

   return getPort(p->getModuleID(), p->getName());
}

bool PortTracker::addConnection(const Port *from, const Port *to) {

   Port *f = getPort(from);
   Port *t = getPort(to);

   assert(f);
   assert(t);

   if (from->getType() == Port::OUTPUT && to->getType() == Port::INPUT) {

      f->addConnection(t);
      t->addConnection(f);

      return true;
   }

   if (from->getType() == Port::PARAMETER && to->getType() == Port::PARAMETER) {

      f->addConnection(t);
      t->addConnection(f);

      return true;
   }

#ifdef DEBUG
   std::cerr << "PortTracker::addConnection: incompatible types: "
      << from->getModuleID() << ":" << from->getName() << " (" << from->getType() << ") "
      << to->getModuleID() << ":" << to->getName() << " (" << to->getType() << ")" << std::endl;
#endif

   return false;
}

bool PortTracker::addConnection(const int a, const std::string & na,
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

bool PortTracker::removeConnection(const Port *from, const Port *to) {

   Port *f = getPort(from);
   if (!f)
      return false;

   Port *t = getPort(to);
   if (!t)
      return false;

   if ((from->getType() == Port::OUTPUT && to->getType() == Port::INPUT)
      || (from->getType() == Port::PARAMETER && to->getType() == Port::PARAMETER)) {

      f->removeConnection(to);
      t->removeConnection(from);

      return true;
   }

   return false;
}

bool PortTracker::removeConnection(const int a, const std::string & na,
      const int b, const std::string & nb) {

   Port *from = getPort(a, na);
   Port *to = getPort(b, nb);

   if (!from || !to)
      return false;

   return removeConnection(from, to);
}

const Port::PortSet *
PortTracker::getConnectionList(const Port * port) const {

   return getConnectionList(port->getModuleID(), port->getName());
}

const Port::PortSet *
PortTracker::getConnectionList(const int moduleID,
                               const std::string & name) const {

   const Port *port = getPort(moduleID, name);
   if (!port)
      return NULL;
   return &port->connections();
}

std::vector<std::string> PortTracker::getPortNames(const int moduleID, Port::Type type) const {

   std::vector<std::string> result;

   typedef std::map<std::string, Port *> PortMap;
   typedef std::map<int, PortMap *> ModulePortMap;

   ModulePortMap::const_iterator mports = m_ports.find(moduleID);
   if (mports == m_ports.end())
      return result;
   const auto &portOrderIt = m_portOrders.find(moduleID);
   if (portOrderIt == m_portOrders.end())
      return result;

   const PortMap &portmap = *mports->second;
   const PortOrder &portorder = *portOrderIt->second;
   for(PortOrder::const_iterator it = portorder.begin();
         it != portorder.end();
         ++it) {

      const std::string &name = it->second;
      const auto &it2 = portmap.find(name);
      assert(it2 != portmap.end());

      if (type == Port::ANY || it2->second->getType() == type)
         result.push_back(name);
   }

   return result;
}

std::vector<std::string> PortTracker::getInputPortNames(const int moduleID) const {

   return getPortNames(moduleID, Port::INPUT);
}

std::vector<std::string> PortTracker::getOutputPortNames(const int moduleID) const {

   return getPortNames(moduleID, Port::OUTPUT);
}

std::vector<Port *> PortTracker::getPorts(const int moduleID, Port::Type type, bool connectedOnly) const
{
   std::vector<Port *> result;

   typedef std::map<std::string, Port *> PortMap;
   typedef std::map<int, PortMap *> ModulePortMap;

   ModulePortMap::const_iterator mports = m_ports.find(moduleID);
   if (mports == m_ports.end())
      return result;
   const auto &portOrderIt = m_portOrders.find(moduleID);
   if (portOrderIt == m_portOrders.end())
      return result;

   const PortMap &portmap = *mports->second;
   const PortOrder &portorder = *portOrderIt->second;
   for(PortOrder::const_iterator it = portorder.begin();
         it != portorder.end();
         ++it) {

      const std::string &name = it->second;
      const auto &it2 = portmap.find(name);
      assert(it2 != portmap.end());

      if (type == Port::ANY || it2->second->getType() == type) {
         if (!connectedOnly || !it2->second->connections().empty()) {
            result.push_back(it2->second);
         }
      }
   }

   return result;
}

std::vector<Port *> PortTracker::getInputPorts(const int moduleID) const {

   return getPorts(moduleID, Port::INPUT);
}

std::vector<Port *> PortTracker::getConnectedInputPorts(const int moduleID) const {

   return getPorts(moduleID, Port::INPUT, true);
}

std::vector<Port *> PortTracker::getOutputPorts(const int moduleID) const {

   return getPorts(moduleID, Port::OUTPUT);
}

std::vector<Port *> PortTracker::getConnectedOutputPorts(const int moduleID) const {

   return getPorts(moduleID, Port::OUTPUT, true);
}

std::vector<message::Buffer> PortTracker::removeConnectionsWithModule(int moduleId) {

   typedef std::map<std::string, Port *> PortMap;
   typedef std::map<int, PortMap *> ModulePortMap;

   std::vector<message::Buffer> ret;

   ModulePortMap::const_iterator mports = m_ports.find(moduleId);
   if (mports == m_ports.end())
      return ret;

   const PortMap &portmap = *mports->second;
   for(PortMap::const_iterator it = portmap.begin();
         it != portmap.end();
         ++it) {

      Port *port = it->second;
      const Port::PortSet &cl = port->connections();
      while (!cl.empty()) {
         size_t oldsize = cl.size();
         const Port *other = *cl.begin();
         removeConnection(port, other);
         removeConnection(other, port);
         message::Disconnect d1(port->getModuleID(), port->getName(), other->getModuleID(), other->getName());
         ret.push_back(d1);
         message::Disconnect d2(other->getModuleID(), other->getName(), port->getModuleID(), port->getName());
         ret.push_back(d2);
         if (cl.size() == oldsize) {
            std::cerr << "failed to remove all connections for module " << moduleId << ", still left: " << cl.size() << std::endl;
            for (size_t i=0; i<cl.size(); ++i) {
               std::cerr << "   " << port->getModuleID() << ":" << port->getName() << " <--> " << other->getModuleID() << ":" << other->getName() << std::endl;
            }
            break;
         }
      }
   }
   return ret;
}

} // namespace vistle
