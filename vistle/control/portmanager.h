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

   void addObject(const Port *port);
   bool hasObject(const Port *port);
   void popObject(const Port *port);

   void resetInput(const Port *port);
   bool isReset(const Port *port);

   void finishInput(const Port *port);
   bool isFinished(const Port *port);

 private:

   ClusterManager *m_clusterManager;

   std::map<const Port *, int> m_numObject, m_numReset, m_numFinish;
};
} // namespace vistle

#endif
