#ifndef PORT_H
#define PORT_H

#include <string>
#include <vector>
#include <set>
#include <deque>
#include <ostream>
#include <memory>

#include <util/enum.h>
#include "export.h"

namespace vistle {

class Object;
typedef std::shared_ptr<const Object> obj_const_ptr;
typedef std::deque<obj_const_ptr> ObjectList;

template <class T>
struct deref_compare: std::binary_function<T*, T*, bool> {
   bool operator() (const T *x, const T *y) const {
      return *x < *y;
   }
};

class V_COREEXPORT Port {

   friend class PortTracker;

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Type, (ANY) (INPUT) (OUTPUT) (PARAMETER))
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Flags,
      (NONE)
      (MULTI) //< additional ports are created for each connected port
      (COMBINE) //< several connections to a single port are allowed and should be processed together
   )

   Port(int moduleID, const std::string &name, Port::Type type, int flags=0);
   void setDescription(const std::string &desc);
   int getModuleID() const;
   const std::string & getName() const;
   const std::string & getDescription() const;
   Type getType() const;
   int flags() const;

   ObjectList &objects();
   const ObjectList &objects() const;

   typedef std::set<Port *, deref_compare<Port>> PortSet;
   typedef std::set<const Port *, deref_compare<Port>> ConstPortSet;
   const ConstPortSet &connections() const;
   void setConnections(const ConstPortSet &conn);
   bool addConnection(Port *other);
   const Port *removeConnection(const Port &other);
   bool isConnected() const;

   const PortSet &linkedPorts() const;

   bool operator<(const Port &other) const {
      if (getModuleID() == other.getModuleID()) {
          return getName() < other.getName();
      }
      return getModuleID() < other.getModuleID();
   }

   //! children of 'MULTI' ports
   Port *child(size_t idx, bool link=false);
   //! input and output ports can be linked if there is dependency between them
   bool link(Port *linked);
   
 private:
   int moduleID;
   std::string name;
   std::string description;
   Type type;
   int m_flags;
   ObjectList m_objects;
   ConstPortSet m_connections;
   Port *m_parent;
   std::vector<Port *> m_children;
   PortSet m_linkedPorts;
};

V_COREEXPORT std::ostream &operator<<(std::ostream &s, const Port &port);

V_ENUM_OUTPUT_OP(Type, Port)
V_ENUM_OUTPUT_OP(Flags, Port)

} // namespace vistle
#endif
