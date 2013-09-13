#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include <string>

#include <core/porttracker.h>

namespace vistle {

class ModuleManager;

class PortManager: public PortTracker {

 public:
   PortManager(ModuleManager *moduleManager);
   virtual ~PortManager();

   virtual Port * getPort(const int moduleID, const std::string & name) const;
   void removeConnections(const int moduleID);

 private:

   ModuleManager *m_moduleManager;
};
} // namespace vistle

#endif
