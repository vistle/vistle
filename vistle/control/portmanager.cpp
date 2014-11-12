#include "portmanager.h"
#include "clustermanager.h"
#include <core/message.h>
#include <iostream>
#include <algorithm>

namespace vistle {

PortManager::PortManager(ClusterManager *clusterManager)
: m_clusterManager(clusterManager)
{
}

PortManager::~PortManager() {
}

Port * PortManager::getPort(const int moduleID,
                            const std::string & name) const {

   if (Port *p = PortTracker::getPort(moduleID, name))
      return p;

   std::string::size_type p = name.find('[');
   if (p != std::string::npos) {
      Port *parent = getPort(moduleID, name.substr(0, p-1));
      if (parent && (parent->flags() & Port::MULTI)) {
         size_t idx=boost::lexical_cast<size_t>(name.substr(p+1));
         Port *port = parent->child(idx);
         m_clusterManager->sendMessage(moduleID, message::AddPort(port));
         return port;
      }
   }

   return NULL;
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
      const Port::PortSet &cl = port->connections();
      while (!cl.empty()) {
         size_t oldsize = cl.size();
         const Port *other = *cl.begin();
         removeConnection(port, other);
         removeConnection(other, port);
         message::Disconnect d1(port->getModuleID(), port->getName(), other->getModuleID(), other->getName());
         m_clusterManager->sendAll(d1);
         m_clusterManager->sendUi(d1);
         message::Disconnect d2(other->getModuleID(), other->getName(), port->getModuleID(), port->getName());
         m_clusterManager->sendAll(d2);
         m_clusterManager->sendUi(d2);
         if (cl.size() == oldsize) {
            std::cerr << "failed to remove all connections for module " << moduleID << ", still left: " << cl.size() << std::endl;
            for (size_t i=0; i<cl.size(); ++i) {
               std::cerr << "   " << port->getModuleID() << ":" << port->getName() << " <--> " << other->getModuleID() << ":" << other->getName() << std::endl;
            }
            break;
         }
      }
   }
}

void PortManager::addObject(const Port *port) {

   ++m_numObject[port];
}

bool PortManager::hasObject(const Port *port) {

   return m_numObject[port] > 0;
}

void PortManager::popObject(const Port *port) {

   vassert(m_numObject[port] > 0);
   --m_numObject[port];
}

void PortManager::resetInput(const Port *port) {

   ++m_numReset[port];
}

bool PortManager::isReset(const Port *port) {

   return m_numReset[port] > 0;
}

void PortManager::popReset(const Port *port) {

   --m_numReset[port];
}

void PortManager::finishInput(const Port *port) {

   vassert(m_numObject[port] == 0);
   ++m_numFinish[port];
}

bool PortManager::isFinished(const Port *port) {

   vassert(m_numObject[port] == 0);
   return m_numFinish[port] > 0;
}

void PortManager::popFinish(const Port *port) {

   --m_numFinish[port];
}


} // namespace vistle
