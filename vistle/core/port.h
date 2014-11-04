#ifndef PORT_H
#define PORT_H

#include <string>
#include <vector>
#include <set>

#include <util/enum.h>
#include "objectcache.h"
#include "export.h"

namespace vistle {

template <class T>
struct deref_compare: std::binary_function<T*, T*, bool> {
   bool operator() (const T *x, const T *y) const {
      return *x < *y;
   }
};

class V_COREEXPORT Port {

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Type, (ANY) (INPUT) (OUTPUT) (PARAMETER))
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Flags,
      (NONE)
      (MULTI) //< additional ports are created for each connected port
      (COMBINE) //< several connections to a single port are allowed and should be processed together
   )

   Port(int moduleID, const std::string &name, Port::Type type, int flags=0);
   int getModuleID() const;
   const std::string & getName() const;
   Type getType() const;
   int flags() const;

   ObjectList &objects();
   const ObjectList &objects() const;

   typedef std::set<Port *, deref_compare<Port> > PortSet;
   const PortSet &connections() const;
   void setConnections(const PortSet &conn);
   void addConnection(Port *other);
   Port *removeConnection(const Port *other);
   bool isConnected() const;

   const PortSet &linkedPorts() const;

   bool operator<(const Port &other) const {
      if (getModuleID() < other.getModuleID())
         return true;
      return getName() < other.getName();
   }

   //! children of 'MULTI' ports
   Port *child(size_t idx, bool link=false);
   //! input and output ports can be linked if there is dependency between them
   bool link(Port *other);
   
 private:
   const int moduleID;
   const std::string name;
   const Type type;
   const int m_flags;
   ObjectList m_objects;
   PortSet m_connections;
   Port *m_parent;
   std::vector<Port *> m_children;
   PortSet m_linkedPorts;
};

V_ENUM_OUTPUT_OP(Type, Port)
V_ENUM_OUTPUT_OP(Flags, Port)

} // namespace vistle
#endif
