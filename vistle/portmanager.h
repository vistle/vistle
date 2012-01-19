#ifndef PORTMANAGER_H
#define PORTMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

class Port {

 public:
   enum Type { INPUT = 1, OUTPUT = 2 };

   Port(int moduleID, const std::string &name, const Port::Type type);
   int getModuleID() const;
   const std::string & getName() const;
   Type getType() const;

   void addObject(const shm_handle_t & handle);

 private:
   const int moduleID;
   const std::string name;
   const Type type;

   std::vector<shm_handle_t> objects;
};


class PortManager {

 public:
   PortManager();
   Port * addPort(const int moduleID, const std::string & name,
                  const Port::Type type);

   void addConnection(const Port * out, const Port * in);
   void addConnection(const int a, const std::string & na,
                      const int b, const std::string & nb);

   const std::vector<const Port *> * getConnectionList(const Port * port) const;
   const std::vector<const Port *> * getConnectionList(const int moduleID,
                                                       const std::string & name)
      const;

   Port * getPort(const int moduleID, const std::string & name) const;

 private:

   // module ID -> list of ports belonging to the module
   std::map<int, std::map<std::string, Port *> *> ports;

   // port -> list of ports that the port is connected to
   std::map<const Port *, std::vector<const Port *> *> connections;
};
} // namespace vistle

#endif
