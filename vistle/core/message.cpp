#include "message.h"
#include "shm.h"
#include "parameter.h"

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

      assert(size < MESSAGE_SIZE);
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
             const std::string & n, int debugFlag, int debugRank)
   : Message(moduleID, rank, Message::SPAWN, sizeof(Spawn))
   , spawnID(s)
   , debugFlag(debugFlag)
   , debugRank(debugRank)
{

   COPY_STRING(name, n);
}

int Spawn::getSpawnID() const {

   return spawnID;
}

const char * Spawn::getName() const {

   return name;
}

Started::Started(const int moduleID, const int rank, const std::string &n)
: Message(moduleID, rank, Message::STARTED, sizeof(Started))
{

   COPY_STRING(name, n);
}

const char * Started::getName() const {

   return name;
}

int Spawn::getDebugFlag() const {

   return debugFlag;
}

int Spawn::getDebugRank() const {

   return debugRank;
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

Busy::Busy(const int moduleID, const int rank)
   : Message(moduleID, rank, Message::BUSY, sizeof(Busy)) {
}

Idle::Idle(const int moduleID, const int rank)
   : Message(moduleID, rank, Message::IDLE, sizeof(Idle)) {
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

const char * AddObject::getPortName() const {

   return portName;
}

const shm_handle_t & AddObject::getHandle() const {

   return handle;
}

Object::const_ptr AddObject::takeObject() const {

   vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
   if (obj)
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

AddParameter::AddParameter(const int moduleID, const int rank,
      const std::string &n, int t)
: Message(moduleID, rank, Message::ADDPARAMETER, sizeof(AddParameter))
, paramtype(t) {

   assert(paramtype > Parameter::Unknown);
   assert(paramtype < Parameter::Invalid);

   COPY_STRING(name, n);
}

const char *AddParameter::getName() const {

   return name;
}

int AddParameter::getParameterType() const {

   return paramtype;
}

Parameter *AddParameter::getParameter() const {

   switch (getParameterType()) {
      case Parameter::Integer:
         return new IntParameter(getName());
      case Parameter::Scalar:
         return new FloatParameter(getName());
      case Parameter::Vector:
         return new VectorParameter(getName());
      case Parameter::String:
         return new StringParameter(getName());
      case Parameter::Invalid:
      case Parameter::Unknown:
         break;
   }

   std::cerr << "AddParameter::getParameter: type " << getType() << " not handled" << std::endl;
   assert("parameter type not supported" == 0);

   return NULL;
}

SetParameter::SetParameter(const int moduleID, const int rank, const int module,
      const std::string &n, const Parameter *param)
: Message(moduleID, rank, Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(param->type()) {

   COPY_STRING(name, n);
   if (const IntParameter *pint = dynamic_cast<const IntParameter *>(param)) {
      v_int = pint->getValue();
   } else if (const FloatParameter *pfloat = dynamic_cast<const FloatParameter *>(param)) {
      v_scalar = pfloat->getValue();
   } else if (const VectorParameter *pvec = dynamic_cast<const VectorParameter *>(param)) {
      Vector v = pvec->getValue();
      dim = v.dim;
      for (int i=0; i<MaxDimension; ++i)
         v_vector[i] = v[i];
   } else if (const StringParameter *pstring = dynamic_cast<const StringParameter *>(param)) {
      COPY_STRING(v_string, pstring->getValue());
   } else {
      std::cerr << "SetParameter: type " << param->type() << " not handled" << std::endl;
      assert("invalid parameter type" == 0);
   }
}

SetParameter::SetParameter(const int moduleID, const int rank, const int module,
      const std::string &n, const int v)
: Message(moduleID, rank, Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Integer) {

   COPY_STRING(name, n);
   v_int = v;
}

SetParameter::SetParameter(const int moduleID, const int rank, const int module,
      const std::string &n, const Scalar v)
: Message(moduleID, rank, Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Scalar) {

   COPY_STRING(name, n);
   v_scalar = v;
}

SetParameter::SetParameter(const int moduleID, const int rank, const int module,
      const std::string &n, const Vector v)
: Message(moduleID, rank, Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Vector) {

   COPY_STRING(name, n);
   dim = v.dim;
   for (int i=0; i<MaxDimension; ++i)
      v_vector[i] = v[i];
}

SetParameter::SetParameter(const int moduleID, const int rank, const int module,
      const std::string &n, const std::string &v)
: Message(moduleID, rank, Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::String) {

   COPY_STRING(name, n);
   COPY_STRING(v_string, v);
}

const char *SetParameter::getName() const {

   return name;
}

int SetParameter::getModule() const {

   return module;
}

int SetParameter::getParameterType() const {

   return paramtype;
}

int SetParameter::getInteger() const {

   assert(paramtype == Parameter::Integer);
   return v_int;
}

Scalar SetParameter::getScalar() const {

   assert(paramtype == Parameter::Scalar);
   return v_scalar;
}

Vector SetParameter::getVector() const {

   assert(paramtype == Parameter::Vector);
   return Vector(dim, &v_vector[0]);
}

std::string SetParameter::getString() const {

   assert(paramtype == Parameter::String);
   return v_string;
}

bool SetParameter::apply(Parameter *param) const {

   if (paramtype != param->type()) {
      std::cerr << "SetParameter::apply(): type mismatch" << std::endl;
      return false;
   }

   if (IntParameter *pint = dynamic_cast<IntParameter *>(param)) {
      pint->setValue(v_int);
   } else if (FloatParameter *pfloat = dynamic_cast<FloatParameter *>(param)) {
      pfloat->setValue(v_scalar);
   } else if (VectorParameter *pvec = dynamic_cast<VectorParameter *>(param)) {
      pvec->setValue(Vector(dim, &v_vector[0]));
   } else if (StringParameter *pstring = dynamic_cast<StringParameter *>(param)) {
      pstring->setValue(v_string);
   } else {
      std::cerr << "SetParameter::apply(): type " << param->type() << " not handled" << std::endl;
      assert("invalid parameter type" == 0);
   }
   
   return true;
}

Barrier::Barrier(const int moduleID, const int rank, const int id)
: Message(moduleID, rank, Message::BARRIER, sizeof(Barrier))
, barrierid(id)
{
}

int Barrier::getBarrierId() const {

   return barrierid;
}

BarrierReached::BarrierReached(const int moduleID, const int rank, const int id)
: Message(moduleID, rank, Message::BARRIERREACHED, sizeof(BarrierReached))
, barrierid(id)
{
}

int BarrierReached::getBarrierId() const {

   return barrierid;
}

} // namespace message
} // namespace vistle
