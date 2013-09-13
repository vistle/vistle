#include "portmanager.h"
#include "modulemanager.h"
#include <core/message.h>
#include <iostream>
#include <algorithm>

namespace vistle {

PortManager::PortManager(ModuleManager *moduleManager)
: m_moduleManager(moduleManager)
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
         std::stringstream idxstr(name.substr(p+1));
         size_t idx=0;
         idxstr >> idx;
         Port *port = parent->child(idx);
         m_moduleManager->sendMessage(moduleID, message::CreatePort(port));
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
         m_moduleManager->sendAll(d1);
         m_moduleManager->sendUi(d1);
         message::Disconnect d2(other->getModuleID(), other->getName(), port->getModuleID(), port->getName());
         m_moduleManager->sendAll(d2);
         m_moduleManager->sendUi(d2);
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

} // namespace vistle
