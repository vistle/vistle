#include "porttracker.h"
#include "statetracker.h"
#include "parameter.h"
#include <core/message.h>
#include <iostream>
#include <algorithm>

#define CERR std::cerr << (m_stateTracker ? m_stateTracker->m_name : "(null)") << ": "

namespace vistle {

PortTracker::PortTracker()
: m_stateTracker(nullptr)
{
}

PortTracker::~PortTracker() {
}

void PortTracker::setTracker(StateTracker *tracker) {

    m_stateTracker = tracker;
}

StateTracker *PortTracker::tracker() const {

   return m_stateTracker;
}

Port * PortTracker::addPort(const int moduleID, const std::string & name,
                          const Port::Type type) {
   Port *port = new Port(moduleID, name, type);
   return addPort(port);
}

Port *PortTracker::addPort(Port *port) {

   assert(port->getType() != Port::ANY);

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

      if (port->getType() == Port::INPUT) {
         const bool combine = port->flags() & Port::COMBINE;
         for (auto &p: *portMap) {
            if (p.second->getType() == Port::INPUT) {
               if ((port->flags() & Port::COMBINE) || combine) {
                  CERR << "COMBINE port has to be only input port of a module" << std::endl;
                  delete port;
                  return nullptr;
               }
            }
         }
      }
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

   bool added = false;
   if (f->getType() == Port::OUTPUT && t->getType() == Port::INPUT) {

      if (!t->isConnected() || (t->flags() & Port::COMBINE)) {
         if (f->addConnection(t)) {
            if (t->addConnection(f)) {
               added = true;
            } else {
               f->removeConnection(t);
               CERR << "PortTracker::addConnection: inconsistent connection states for data connection" << std::endl;
               return true;
            }
         }
      } else {
         //CERR << "PortTracker::addConnection: multiple connection" << std::endl;
         return true;
      }
   } else if (f->getType() == Port::PARAMETER && t->getType() == Port::PARAMETER) {

      if (f->addConnection(t)) {
         if (t->addConnection(f)) {
            added = true;
         } else {
            f->removeConnection(t);
            CERR << "PortTracker::addConnection: inconsistent connection states for parmeter connection" << std::endl;
            return true;
         }
      }
   } else {
      CERR << "PortTracker::addConnection: incompatible types: "
         << from->getModuleID() << ":" << from->getName() << " (" << from->getType() << ") "
         << to->getModuleID() << ":" << to->getName() << " (" << to->getType() << ")" << std::endl;

      return true;
   }

   if (added) {
      if (tracker()) {
         for (StateObserver *o: tracker()->m_observers) {
            o->incModificationCount();
            o->newConnection(f->getModuleID(), f->getName(),
                                t->getModuleID(), t->getName());
         }
      }
      return true;
   }

#ifdef DEBUG
   CERR << "PortTracker::addConnection: have to queue connection: "
      << from->getModuleID() << ":" << from->getName() << " (" << from->getType() << ") "
      << to->getModuleID() << ":" << to->getName() << " (" << to->getType() << ")" << std::endl;
#endif

   return false;
}

bool PortTracker::addConnection(const int a, const std::string & na,
                                const int b, const std::string & nb) {

   Port *portA = getPort(a, na);
   if (!portA) {
      return false;
   }
   Port *portB = getPort(b, nb);
   if (!portB) {
      return false;
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

   const auto ft = f->getType();
   const auto tt = t->getType();
   if (((ft == Port::OUTPUT || ft == Port::ANY) && (tt == Port::INPUT || tt == Port::ANY))
      || ((ft == Port::PARAMETER || ft == Port::ANY) && (tt == Port::PARAMETER || tt ==  Port::ANY))) {

      f->removeConnection(t);
      t->removeConnection(f);

      if (ft == Port::ANY || tt == Port::ANY) {
          CERR << "removeConnection: port type ANY: " << *f << " -> " << *t << std::endl;
      }

      if (tracker()) {
         for (StateObserver *o: tracker()->m_observers) {
            o->incModificationCount();
            o->deleteConnection(f->getModuleID(), f->getName(),
                                t->getModuleID(), t->getName());
         }
      }

      return true;
   } else {
       CERR << "removeConnection: port type mismatch: " << *f << " -> " << *t << std::endl;
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
   const auto ports = getPorts(moduleID, type);
   for (const auto &port: ports)
       result.push_back(port->getName());
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
      const auto &port = it2->second;

      if (type == Port::ANY || port->getType() == type) {
         if (!connectedOnly || !port->connections().empty()) {
            result.push_back(port);
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

std::vector<message::Buffer> PortTracker::removeModule(int moduleId) {

   //CERR << "removing all connections from/to " << moduleId << std::endl;

   std::vector<message::Buffer> ret;

   ModulePortMap::const_iterator mports = m_ports.find(moduleId);
   if (mports != m_ports.end()) {

      const PortMap &portmap = *mports->second;
      for(PortMap::const_iterator it = portmap.begin();
            it != portmap.end();
            ++it) {

         Port *port = it->second;
         const Port::PortSet &cl = port->connections();
         while (!cl.empty()) {
            size_t oldsize = cl.size();
            const Port *other = *cl.begin();
            if (port->getType() != Port::INPUT && removeConnection(port, other)) {
               message::Disconnect d1(port->getModuleID(), port->getName(), other->getModuleID(), other->getName());
               ret.push_back(d1);
            }
            if (port->getType() != Port::OUTPUT && removeConnection(other, port)) {
               message::Disconnect d2(other->getModuleID(), other->getName(), port->getModuleID(), port->getName());
               ret.push_back(d2);
            }
            if (cl.size() == oldsize) {
               CERR << "failed to remove all connections for module " << moduleId << ", still left: " << cl.size() << std::endl;
               for (size_t i=0; i<cl.size(); ++i) {
                  CERR << "   " << *port << " <--> " << *other << std::endl;
               }
               break;
            }
         }
      }

      for(PortMap::const_iterator it = portmap.begin();
            it != portmap.end();
            ++it) {
          delete it->second;
      }
      mports->second->clear();
   }

   for (const auto &mpm: m_ports) {
       const auto &pm = *mpm.second;
       for (const auto &pme: pm) {
           const Port *port = pme.second;
           if (port->getModuleID() == moduleId) {
               CERR << "removeModule " << moduleId << ": " << *port << " still exists" << std::endl;
           }
           const auto &cl = port->connections();
           for (const auto &other: cl) {
               if (other->getModuleID() == moduleId) {
                   CERR << "removeModule " << moduleId << ": " << *other << " still connected to " << *port << std::endl;
               }
           }
       }
   }

   for (const auto &mpm: m_ports) {
       const auto &pm = *mpm.second;
       for (const auto &pme: pm) {
           const Port *port = pme.second;
           if (port->getModuleID() == moduleId) {
               CERR << "removeModule " << moduleId << ": " << *port << " still exists" << std::endl;
           }
           const auto &cl = port->connections();
           for (const auto &other: cl) {
               if (other->getModuleID() == moduleId) {
                   CERR << "removeModule " << moduleId << ": " << *other << " still connected to " << *port << std::endl;
               }
           }
       }
   }

   return ret;
}

void PortTracker::check() const {

   int numInput=0, numOutput=0, numParam=0;
   for (const auto &mpm: m_ports) {
       const auto &pm = *mpm.second;
       for (const auto &pme: pm) {
           const Port *port = pme.second;
           const auto &cl = port->connections();
           if (port->getType() == Port::INPUT && !(port->flags() & Port::COMBINE)) {
               if (cl.size() > 1) {
                   CERR << "FAIL: too many connections: " << *port << std::endl;
               }
           }
           const int moduleId = port->getModuleID();
           for (const auto &other: cl) {
               if (port->getType() == Port::ANY) {
                   CERR << "FAIL: ANY port: " << *port << std::endl;
               }
               if (other->getType() == Port::ANY) {
                   CERR << "FAIL: ANY port: " << *other << std::endl;
               }
               if (port->getType() == Port::INPUT) {
                   if (other->getType() != Port::OUTPUT) {
                       CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                   } else {
                       ++numInput;
                   }
                   if (other->getModuleID() == moduleId) {
                       CERR << "FAIL: from/to same module: " << *port << " <-> " << *other << std::endl;
                   }
               } else if (port->getType() == Port::OUTPUT) {
                   if (other->getType() != Port::INPUT) {
                       CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                   } else {
                       ++numOutput;
                   }
                   if (other->getModuleID() == moduleId) {
                       CERR << "FAIL: from/to same module: " << *port << " <-> " << *other << std::endl;
                   }
               } else if (port->getType() == Port::PARAMETER) {
                   if (other->getType() != Port::PARAMETER) {
                       CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
                   } else {
                       ++numParam;
                   }
               } else {
                   CERR << "FAIL: connection mismatch: " << *port << " <-> " << *other << std::endl;
               }
           }
       }
   }

   if (numInput != numOutput) {
       CERR << "FAIL: numInput=" << numInput << ", numOutput=" << numOutput << std::endl;
   }
   if (numParam % 2) {
       CERR << "FAIL: numParam=" << numParam << std::endl;
   }
}

} // namespace vistle
