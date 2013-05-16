#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <set>

#include <core/port.h>

namespace vistle {

class PortManager {

 public:
   PortManager();
   Port *addPort(const int moduleID, const std::string & name,
                  const Port::Type type);
   Port *addPort(Port *port);

   bool addConnection(const Port * out, const Port * in);
   bool addConnection(const int a, const std::string & na,
                      const int b, const std::string & nb);

   bool removeConnection(const Port *from, const Port *to);
   bool removeConnection(const int a, const std::string & na,
                         const int b, const std::string & nb);

   void removeConnections(const int moduleID);

   typedef std::vector<const Port *> ConnectionList;

   const ConnectionList *getConnectionList(const Port * port) const;
   const ConnectionList *getConnectionList(const int moduleID,
                                                       const std::string & name)
      const;

   Port * getPort(const int moduleID, const std::string & name) const;

   std::vector<std::string> getPortNames(const int moduleID, Port::Type type) const;
   std::vector<std::string> getInputPortNames(const int moduleID) const;
   std::vector<std::string> getOutputPortNames(const int moduleID) const;

 private:

   typedef std::map<std::string, Port *> PortMap;

   // module ID -> list of ports belonging to the module
   std::map<int, PortMap *> ports;

   // port -> list of ports that the port is connected to
   std::map<const Port *, ConnectionList *> connections;
};
} // namespace vistle

#endif
