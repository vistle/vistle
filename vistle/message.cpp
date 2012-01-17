#include "message.h"

namespace vistle {
namespace message {

Message::Message(const int m, const int r,
                 const Type t, const unsigned int s)
   : moduleID(m), rank(r), size(s), type(t) {

}

int Message::getModuleID() const {

   return moduleID;
}

int Message::getRank() const {

   return rank;
}

Message::Type Message::getType() const {

   return type;
}

Debug::Debug(const int moduleID, const int rank, const char c)
   : Message(moduleID, rank, Message::DEBUG, sizeof(Debug)), character(c) {

}

char Debug::getCharacter() const {

   return character;
}

Spawn::Spawn(const int moduleID, const int rank, const int s)
   : Message(moduleID, rank, Message::SPAWN, sizeof(Spawn)), spawnID(s) {

}

int Spawn::getSpawnID() const {

   return spawnID;
}

Quit::Quit(const int moduleID, const int rank)
   : Message(moduleID, rank, Message::QUIT, sizeof(Quit)) {
}

NewObject::NewObject(const int moduleID, const int rank,
                     const shm_handle_t & h)
   : Message(moduleID, rank, Message::NEWOBJECT, sizeof(NewObject)),
     handle(h) {

}

const shm_handle_t & NewObject::getHandle() const {

   return handle;
}

ModuleExit::ModuleExit(const int moduleID, const int rank)
   : Message(moduleID, rank, Message::MODULEEXIT, sizeof(ModuleExit)) {

}

Compute::Compute(const int moduleID, const int rank)
   : Message(moduleID, rank, Message::COMPUTE, sizeof(Compute)) {
}

CreateOutputPort::CreateOutputPort(const int moduleID, const int rank,
                                   const std::string &n)
   : Message(moduleID, rank,
             Message::CREATEOUTPUTPORT, sizeof(CreateOutputPort)) {

   size_t size = n.size();
   if (size > 31)
      size = 31;
   n.copy(name, size);
   name[size] = 0;
}

const char * CreateOutputPort::getName() const {

   return name;
}

CreateInputPort::CreateInputPort(const int moduleID, const int rank,
                                 const std::string &n)
   : Message(moduleID, rank, Message::CREATEINPUTPORT,
             sizeof(CreateInputPort)) {

   size_t size = n.size();
   if (size > 31)
      size = 31;
   n.copy(name, size);
   name[size] = 0;
}

const char * CreateInputPort::getName() const {

   return name;
}

AddObject::AddObject(const int moduleID, const int rank, const std::string &p,
                     const shm_handle_t & h)
   : Message(moduleID, rank, Message::ADDOBJECT, sizeof(AddObject)), handle(h) {

   size_t size = p.size();
   if (size > 31)
      size = 31;
   p.copy(portName, size);
   portName[size] = 0;
}

const char * AddObject::getPortName() const {

   return portName;
}

const shm_handle_t & AddObject::getHandle() const {

   return handle;
}

} // namespace message
} // namespace vistle
