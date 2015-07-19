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
std::vector<message::Buffer> PortManager::removeConnectionsWithModule(const int moduleID) {

   std::vector<message::Buffer> msgs = PortTracker::removeConnectionsWithModule(moduleID);
   for (const auto &msg: msgs)
      m_clusterManager->sendAll(msg);
   return msgs;
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

   ++m_numFinish[port];
}

bool PortManager::isFinished(const Port *port) {

   return m_numFinish[port] > 0;
}

void PortManager::popFinish(const Port *port) {

   --m_numFinish[port];
}


} // namespace vistle
