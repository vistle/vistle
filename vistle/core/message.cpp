#include "message.h"
#include "shm.h"

namespace vistle {
namespace message {

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

#define COPY_STRING(dst, src) \
   { \
      size_t size = min(src.size(), sizeof(dst)-1); \
      src.copy(dst, size); \
      dst[size] = '\0'; \
      assert(src.size() <= sizeof(dst)-1); \
   }

Message::Message(const int m, const int r,
                 const Type t, const unsigned int s)
   : size(s), type(t), moduleID(m), rank(r) {

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

size_t Message::getSize() const {

   return size;
}

Ping::Ping(const int moduleID, const int rank, const char c)
   : Message(moduleID, rank, Message::PING, sizeof(Ping)), character(c) {

}

char Ping::getCharacter() const {

   return character;
}

Pong::Pong(const int moduleID, const int rank, const char c, const int module)
   : Message(moduleID, rank, Message::PONG, sizeof(Pong)), character(c) {

}

char Pong::getCharacter() const {

   return character;
}

int Pong::getDestination() const {

   return module;
}

Spawn::Spawn(const int moduleID, const int rank, const int s,
             const std::string & n)
   : Message(moduleID, rank, Message::SPAWN, sizeof(Spawn)), spawnID(s) {

      COPY_STRING(name, n);
}

int Spawn::getSpawnID() const {

   return spawnID;
}

const char * Spawn::getName() const {

   return name;
}

Kill::Kill(const int moduleID, const int rank, const int m)
   : Message(moduleID, rank, Message::KILL, sizeof(Kill)), module(m) {
}

int Kill::getModule() const {

   return module;
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

      COPY_STRING(name, n);
}

const char * CreateOutputPort::getName() const {

   return name;
}

CreateInputPort::CreateInputPort(const int moduleID, const int rank,
                                 const std::string & n)
   : Message(moduleID, rank, Message::CREATEINPUTPORT,
             sizeof(CreateInputPort)) {

      COPY_STRING(name, n);
}

const char * CreateInputPort::getName() const {

   return name;
}

AddObject::AddObject(const int moduleID, const int rank, const std::string & p,
                     vistle::Object::const_ptr obj)
   : Message(moduleID, rank, Message::ADDOBJECT, sizeof(AddObject)),
     handle(obj->getHandle()) {
        // we keep the handle as a reference to obj
        obj->ref();

      COPY_STRING(portName, p);
}

#if 0
AddObject::~AddObject() {
   vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
   // the reference through handle goes away
   obj->unref();
}
#endif

const char * AddObject::getPortName() const {

   return portName;
}

const shm_handle_t & AddObject::getHandle() const {

   return handle;
}

Object::const_ptr AddObject::takeObject() const {

   vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
   obj->unref();
   return obj;
}

Connect::Connect(const int moduleID, const int rank,
                 const int moduleIDA, const std::string & portA,
                 const int moduleIDB, const std::string & portB)
   : Message(moduleID, rank, Message::CONNECT, sizeof(Connect)),
     moduleA(moduleIDA), moduleB(moduleIDB) {

        COPY_STRING(portAName, portA);
        COPY_STRING(portBName, portB);
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
             sizeof(AddFileParameter)) {

      COPY_STRING(name, n);
      COPY_STRING(value, v);
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
             sizeof(SetFileParameter)), module(m) {

      COPY_STRING(name, n);
      COPY_STRING(value, v);
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
                                     const vistle::Scalar v)
   : Message(moduleID, rank, Message::ADDFLOATPARAMETER,
             sizeof(AddFloatParameter)), value(v) {

      COPY_STRING(name, n);
}

const char * AddFloatParameter::getName() const {

   return name;
}

vistle::Scalar AddFloatParameter::getValue() const {

   return value;
}

SetFloatParameter::SetFloatParameter(const int moduleID, const int rank,
                                     const int m, const std::string & n,
                                     const vistle::Scalar v)
   : Message(moduleID, rank, Message::SETFLOATPARAMETER,
             sizeof(SetFloatParameter)), module(m), value(v) {

      COPY_STRING(name, n);
}

int SetFloatParameter::getModule() const {

   return module;
}

const char * SetFloatParameter::getName() const {

   return name;
}

vistle::Scalar SetFloatParameter::getValue() const {

   return value;
}

AddIntParameter::AddIntParameter(const int moduleID, const int rank,
                                 const std::string & n,
                                 const int v)
   : Message(moduleID, rank, Message::ADDINTPARAMETER,
             sizeof(AddIntParameter)), value(v) {

      COPY_STRING(name, n);
}

const char * AddIntParameter::getName() const {

   return name;
}

int AddIntParameter::getValue() const {

   return value;
}

SetIntParameter::SetIntParameter(const int moduleID, const int rank,
                                 const int m, const std::string & n,
                                 const int v)
   : Message(moduleID, rank, Message::SETINTPARAMETER,
             sizeof(SetIntParameter)), module(m), value(v) {

      COPY_STRING(name, n);
}

int SetIntParameter::getModule() const {

   return module;
}

const char * SetIntParameter::getName() const {

   return name;
}

int SetIntParameter::getValue() const {

   return value;
}

AddVectorParameter::AddVectorParameter(const int moduleID, const int rank,
                                       const std::string & n,
                                       const Vector & v)
   : Message(moduleID, rank, Message::ADDVECTORPARAMETER,
             sizeof(AddVectorParameter)), value(v) {

      COPY_STRING(name, n);
}

const char * AddVectorParameter::getName() const {

   return name;
}

Vector AddVectorParameter::getValue() const {

   return value;
}

SetVectorParameter::SetVectorParameter(const int moduleID, const int rank,
                                       const int m, const std::string & n,
                                       const Vector & v)
   : Message(moduleID, rank, Message::SETVECTORPARAMETER,
             sizeof(SetVectorParameter)), module(m), value(v) {

      COPY_STRING(name, n);
}

int SetVectorParameter::getModule() const {

   return module;
}

const char * SetVectorParameter::getName() const {

   return name;
}

Vector SetVectorParameter::getValue() const {

   return value;
}


} // namespace message
} // namespace vistle
