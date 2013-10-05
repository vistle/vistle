#ifndef PORTTRACKER_H
#define PORTTRACKER_H

#include <string>
#include <map>
#include <vector>
#include <set>

#include "export.h"
#include "port.h"

namespace vistle {

class V_COREEXPORT PortTracker {

 public:
   PortTracker();
   virtual ~PortTracker();

   Port *addPort(const int moduleID, const std::string & name,
                  const Port::Type type);
   Port *addPort(Port *port);

   bool addConnection(const Port * out, const Port * in);
   bool addConnection(const int a, const std::string & na,
                      const int b, const std::string & nb);

   bool removeConnection(const Port *from, const Port *to);
   bool removeConnection(const int a, const std::string & na,
                         const int b, const std::string & nb);

   typedef std::vector<const Port *> ConnectionList;

   const Port::PortSet *getConnectionList(const Port * port) const;
   const Port::PortSet *getConnectionList(const int moduleID,
                                                       const std::string & name)
      const;

   virtual Port * getPort(const int moduleID, const std::string & name) const;

   std::vector<std::string> getPortNames(const int moduleID, Port::Type type) const;
   std::vector<std::string> getInputPortNames(const int moduleID) const;
   std::vector<std::string> getOutputPortNames(const int moduleID) const;
   std::vector<Port *> getPorts(const int moduleID, Port::Type type) const;
   std::vector<Port *> getInputPorts(const int moduleID) const;
   std::vector<Port *> getOutputPorts(const int moduleID) const;

 protected:

   Port *getPort(const Port *p) const;

   typedef std::map<int, std::string> PortOrder;
   typedef std::map<std::string, Port *> PortMap;

   // module ID -> list of ports belonging to the module
   std::map<int, PortMap *> m_ports;
   std::map<int, PortOrder *> m_portOrders;
};
} // namespace vistle

#endif
