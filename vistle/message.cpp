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
   if (size > 63)
      size = 63;
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

} // namespace message
} // namespace vistle
