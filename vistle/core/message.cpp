#include "message.h"

namespace vistle {
namespace message {

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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

Spawn::Spawn(const int moduleID, const int rank, const int s,
             const std::string & n)
   : Message(moduleID, rank, Message::SPAWN, sizeof(Spawn)), spawnID(s) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;
}

int Spawn::getSpawnID() const {

   return spawnID;
}

const char * Spawn::getName() const {

   return name;
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

Compute::Compute(const int moduleID, const int rank, const int m)
   : Message(moduleID, rank, Message::COMPUTE, sizeof(Compute)), module(m) {
}

int Compute::getModule() const {

   return module;
}

CreateOutputPort::CreateOutputPort(const int moduleID, const int rank,
                                   const std::string & n)
   : Message(moduleID, rank,
             Message::CREATEOUTPUTPORT, sizeof(CreateOutputPort)) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;
}

const char * CreateOutputPort::getName() const {

   return name;
}

CreateInputPort::CreateInputPort(const int moduleID, const int rank,
                                 const std::string & n)
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

AddObject::AddObject(const int moduleID, const int rank, const std::string & p,
                     const shm_handle_t & h)
   : Message(moduleID, rank, Message::ADDOBJECT, sizeof(AddObject)),
     handle(h) {

   size_t size = MIN(p.size(), 31);
   p.copy(portName, size);
   portName[size] = 0;
}

const char * AddObject::getPortName() const {

   return portName;
}

const shm_handle_t & AddObject::getHandle() const {

   return handle;
}

Connect::Connect(const int moduleID, const int rank,
                 const int moduleIDA, const std::string & portA,
                 const int moduleIDB, const std::string & portB)
   : Message(moduleID, rank, Message::CONNECT, sizeof(Connect)),
     moduleA(moduleIDA), moduleB(moduleIDB) {

   size_t size = MIN(portA.size(), 31);
   portA.copy(portAName, size);
   portAName[size] = 0;

   size = MIN(portB.size(), 31);
   portB.copy(portBName, size);
   portBName[size] = 0;
}

const char * Connect::getPortAName() const {

   return portAName;
}

const char * Connect::getPortBName() const {

   return portBName;
}

int Connect::getModuleA() const {

   return moduleA;
}

int Connect::getModuleB() const {

   return moduleB;
}

AddFileParameter::AddFileParameter(const int moduleID, const int rank,
                                   const std::string & n,
                                   const std::string & v)
   : Message(moduleID, rank, Message::ADDFILEPARAMETER,
             sizeof(ADDFILEPARAMETER)) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;

   size = MIN(v.size(), 255);
   v.copy(value, size);
   value[size] = 0;
}

const char * AddFileParameter::getName() const {

   return name;
}

const char * AddFileParameter::getValue() const {

   return value;
}

SetFileParameter::SetFileParameter(const int moduleID, const int rank,
                                   const int m, const std::string & n,
                                   const std::string & v)
   : Message(moduleID, rank, Message::SETFILEPARAMETER,
             sizeof(SETFILEPARAMETER)), module(m) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;

   size = MIN(v.size(), 255);
   v.copy(value, size);
   value[size] = 0;
}

int SetFileParameter::getModule() const {

   return module;
}

const char * SetFileParameter::getName() const {

   return name;
}

const char * SetFileParameter::getValue() const {

   return value;
}

AddFloatParameter::AddFloatParameter(const int moduleID, const int rank,
                                     const std::string & n,
                                     const float v)
   : Message(moduleID, rank, Message::ADDFLOATPARAMETER,
             sizeof(ADDFLOATPARAMETER)), value(v) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;
}

const char * AddFloatParameter::getName() const {

   return name;
}

float AddFloatParameter::getValue() const {

   return value;
}

SetFloatParameter::SetFloatParameter(const int moduleID, const int rank,
                                     const int m, const std::string & n,
                                     const float v)
   : Message(moduleID, rank, Message::SETFLOATPARAMETER,
             sizeof(SETFLOATPARAMETER)), module(m), value(v) {

   size_t size = MIN(n.size(), 31);
   n.copy(name, size);
   name[size] = 0;
}

int SetFloatParameter::getModule() const {

   return module;
}

const char * SetFloatParameter::getName() const {

   return name;
}

float SetFloatParameter::getValue() const {

   return value;
}

} // namespace message
} // namespace vistle
