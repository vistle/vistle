#ifndef MODULE_IMPL_H
#define MODULE_IMPL_H

#include <core/message.h>
#include <core/messages.h>
#include <core/assert.h>

namespace vistle {

template<class Type>
typename Type::const_ptr Module::accept(Port *port) {
   Object::const_ptr obj;
   if (port->objects().empty()) {
      return nullptr;
   }
   obj = port->objects().front();
   if (!obj) {
      port->objects().pop_front();
      std::stringstream str;
      str << "did not receive valid object at " << port->getName() << std::endl;
      sendWarning(str.str());
      return nullptr;
   }
   vassert(obj->check());
   typename Type::const_ptr ret = Type::as(obj);
   if (ret)
      port->objects().pop_front();
   return ret;
}
template<class Type>
typename Type::const_ptr Module::accept(const std::string &port) {
   Port *p = findInputPort(port);
   vassert(p);
   return accept<Type>(p);
}

template<class Type>
typename Type::const_ptr Module::expect(Port *port) {
   if (!port) {
       std::stringstream str;
       str << "invalid port" << std::endl;
       sendError(str.str());
       return nullptr;
   }
   if (port->objects().empty()) {
      if (schedulingPolicy() == message::SchedulingPolicy::Single) {
          std::stringstream str;
          str << "no object available at " << port->getName() << ", but " << Object::typeName() << " is required" << std::endl;
          sendError(str.str());
      }
      return nullptr;
   }
   Object::const_ptr obj = port->objects().front();
   typename Type::const_ptr ret = Type::as(obj);
   port->objects().pop_front();
   if (!obj) {
      std::stringstream str;
      str << "did not receive valid object at " << port->getName() << ", but " << Object::typeName() << " is required" << std::endl;
      sendError(str.str());
      return ret;
   }
   vassert(obj->check());
   if (!ret) {
      std::stringstream str;
      str << "received " << Object::toString(obj->getType()) << " at " << port->getName() << ", but " << Object::typeName() << " is required" << std::endl;
      sendError(str.str());
   }
   return ret;
}
template<class Type>
typename Type::const_ptr Module::expect(const std::string &port) {
   Port *p = findInputPort(port);
   vassert(p);
   return expect<Type>(p);
}


} // namespace vistle
#endif
