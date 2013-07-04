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

} // namespace vistle
