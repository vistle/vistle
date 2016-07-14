#ifndef MODULE_IMPL_H
#define MODULE_IMPL_H

#include <core/message.h>
#include <core/messages.h>

namespace vistle {

template<class T>
bool Module::setParameter(const std::string &name, const T &value, const message::SetParameter *inResponseTo) {

   auto p = boost::dynamic_pointer_cast<ParameterBase<T>>(findParameter(name));
   if (!p)
      return false;

   return setParameter(p.get(), value, inResponseTo);
}

template<class T>
bool Module::setParameter(ParameterBase<T> *param, const T &value, const message::SetParameter *inResponseTo) {

   param->setValue(value);
   parameterChangedWrapper(param);
   return updateParameter(param->getName(), param, inResponseTo);
}

template<class T>
bool Module::setParameterMinimum(ParameterBase<T> *param, const T &minimum) {

   return Module::setParameterRange(param, minimum, param->maximum());
}

template<class T>
bool Module::setParameterMaximum(ParameterBase<T> *param, const T &maximum) {

   return Module::setParameterRange(param, param->minimum(), maximum);
}

template<class T>
bool Module::setParameterRange(const std::string &name, const T &minimum, const T &maximum) {

   auto p = boost::dynamic_pointer_cast<ParameterBase<T>>(findParameter(name));
   if (!p)
      return false;

   return setParameterRange(p.get(), minimum, maximum);
}

template<class T>
bool Module::setParameterRange(ParameterBase<T> *param, const T &minimum, const T &maximum) {

   bool ok = true;

   if (minimum > maximum) {
      std::cerr << "range for parameter " << param->getName() << " swapped: min " << minimum << " > max " << maximum << std::endl;
      param->setMinimum(maximum);
      param->setMaximum(minimum);
   } else {
      param->setMinimum(minimum);
      param->setMaximum(maximum);
   }
   T value = param->getValue();
   if (value < param->minimum()) {
      std::cerr << "value " << value << " for parameter " << param->getName() << " increased to minimum: " << param->minimum() << std::endl;
      param->setValue(param->minimum());
      ok &= updateParameter(param->getName(), param, nullptr);
   }
   if (value > param->maximum()) {
      std::cerr << "value " << value << " for parameter " << param->getName() << " decreased to maximum: " << param->maximum() << std::endl;
      param->setValue(param->maximum());
      ok &= updateParameter(param->getName(), param, nullptr);
   }
   ok &= updateParameter(param->getName(), param, nullptr, Parameter::Minimum);
   ok &= updateParameter(param->getName(), param, nullptr, Parameter::Maximum);
   return ok;
}

template<class T>
bool Module::getParameter(const std::string &name, T &value) const {

   if (auto p = boost::dynamic_pointer_cast<ParameterBase<T>>(findParameter(name))) {
      value = p->getValue();
   } else {
      std::cerr << "Module::getParameter(" << name << "): type failure" << std::endl;
      vassert("dynamic_cast failed" == 0);
      return false;
   }

   return true;
}

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
   Object::const_ptr obj;
   if (port->objects().empty()) {
      if (schedulingPolicy() == message::SchedulingPolicy::Single) {
          std::stringstream str;
          str << "no object available at " << port->getName() << ", but " << Object::typeName() << " is required" << std::endl;
          sendError(str.str());
      }
      return nullptr;
   }
   obj = port->objects().front();
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
