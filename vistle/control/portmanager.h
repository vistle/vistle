#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include <string>

#include <core/porttracker.h>

#include "export.h"

namespace vistle {

class ClusterManager;

class V_CONTROLEXPORT PortManager: public PortTracker {

 public:
   PortManager(ClusterManager *clusterManager);
   virtual ~PortManager();

   virtual Port * getPort(const int moduleID, const std::string & name) const;
   void removeConnections(const int moduleID);

 private:

   ClusterManager *m_clusterManager;
};
} // namespace vistle

#endif
