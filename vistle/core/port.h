#ifndef PORT_H
#define PORT_H

#include <string>
#include <vector>
#include <set>

#include "module.h"

namespace vistle {

template <class T>
struct deref_compare: std::binary_function<T*, T*, bool> {
   bool operator() (const T *x, const T *y) const {
      return *x < *y;
   }
};

class Port {

 public:
   enum Type { ANY = 0, INPUT = 1, OUTPUT = 2 };

   Port(int moduleID, const std::string &name, const Port::Type type);
   int getModuleID() const;
   const std::string & getName() const;
   Type getType() const;

   ObjectList &objects();

   typedef std::set<Port *, deref_compare<Port> > PortSet;
   PortSet &connections();
   bool isConnected() const;

   bool operator<(const Port &other) const {
      if (getModuleID() < other.getModuleID())
         return true;
      return getName() < other.getName();
   }
   
 private:
   const int moduleID;
   const std::string name;
   const Type type;
   ObjectList m_objects;
   PortSet m_connections;
};

} // namespace vistle
#endif
