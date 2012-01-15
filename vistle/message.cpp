#include "message.h"

namespace vistle {
namespace message {

Message::Message(const unsigned int t, const unsigned int s)
   : size(s), type(t) {

}

unsigned int Message::getType() const {

   return type;
}

Debug::Debug(const char c)
   : Message(Message::DEBUG, sizeof(Debug)), character(c) {

}

char Debug::getCharacter() const {

   return character;
}

Spawn::Spawn(const int m)
   : Message(Message::SPAWN, sizeof(Spawn)), moduleID(m) {

}

int Spawn::getModuleID() const {

   return moduleID;
}

Quit::Quit()
   : Message(Message::QUIT, sizeof(Quit)) {
}

NewObject::NewObject(const std::string &n)
   : Message(Message::NEWOBJECT, sizeof(NewObject)) {

   size_t size = n.size();
   if (size > 31)
      size = 31;
   n.copy(name, size);
   name[size] = 0;
}

const char * NewObject::getName() const {

   return name;
}

ModuleExit::ModuleExit(const int m, const int r)
   : Message(Message::MODULEEXIT, sizeof(ModuleExit)), moduleID(m), rank(r) {

}

int ModuleExit::getModuleID() const {

   return moduleID;
}

int ModuleExit::getRank() const {

   return rank;
}

Compute::Compute()
   : Message(Message::COMPUTE, sizeof(Compute)) {
}

CreateOutputPort::CreateOutputPort(const std::string &n)
   : Message(Message::CREATEOUTPUTPORT, sizeof(CreateOutputPort)) {

   size_t size = n.size();
   if (size > 31)
      size = 31;
   n.copy(name, size);
   name[size] = 0;
}

const char * CreateOutputPort::getName() const {

   return name;
}

CreateInputPort::CreateInputPort(const std::string &n)
   : Message(Message::CREATEINPUTPORT, sizeof(CreateInputPort)) {

   size_t size = n.size();
   if (size > 31)
      size = 31;
   n.copy(name, size);
   name[size] = 0;
}

const char * CreateInputPort::getName() const {

   return name;
}

AddObject::AddObject(const std::string &p, const std::string &o)
   : Message(Message::ADDOBJECT, sizeof(AddObject)) {

   size_t size = p.size();
   if (size > 31)
      size = 31;
   p.copy(portName, size);
   portName[size] = 0;

   size = o.size();
   if (size > 31)
      size = 31;
   o.copy(objectName, size);
   objectName[size] = 0;
}

const char * AddObject::getPortName() const {

   return portName;
}

const char * AddObject::getObjectName() const {

   return portName;
}

} // namespace message
} // namespace vistle
