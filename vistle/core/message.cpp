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

DefaultSender DefaultSender::s_instance;

DefaultSender::DefaultSender()
: m_id(-1)
, m_rank(-1)
{
}

int DefaultSender::id() {

   return s_instance.m_id;
}

int DefaultSender::rank() {

   return s_instance.m_rank;
}

void DefaultSender::init(int id, int rank) {

   s_instance.m_id = id;
   s_instance.m_rank = rank;
}

const DefaultSender &DefaultSender::instance() {

   return s_instance;
}

Message::Message(const Type t, const unsigned int s)
   : m_size(s), m_type(t), m_senderId(DefaultSender::id()), m_rank(DefaultSender::rank()) {

      assert(m_size < MESSAGE_SIZE);

      assert(m_senderId >= 0);
      assert(m_rank >= 0);
}

int Message::senderId() const {

   return m_senderId;
}

void Message::setSenderId(int id) {

   m_senderId = id;
}

int Message::rank() const {

   return m_rank;
}

void Message::setRank(int rank) {
   
   m_rank = rank;
}

Message::Type Message::type() const {

   return m_type;
}

size_t Message::size() const {

   return m_size;
}

Ping::Ping(const char c)
   : Message(Message::PING, sizeof(Ping)), character(c) {

}

char Ping::getCharacter() const {

   return character;
}

Pong::Pong(const char c, const int module)
   : Message(Message::PONG, sizeof(Pong)), character(c) {

}

char Pong::getCharacter() const {

   return character;
}

int Pong::getDestination() const {

   return module;
}

Spawn::Spawn(const int s,
             const std::string & n, int debugFlag, int debugRank)
   : Message(Message::SPAWN, sizeof(Spawn))
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

Started::Started(const std::string &n)
: Message(Message::STARTED, sizeof(Started))
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

Kill::Kill(const int m)
   : Message(Message::KILL, sizeof(Kill)), module(m) {
}

int Kill::getModule() const {

   return module;
}

Quit::Quit()
   : Message(Message::QUIT, sizeof(Quit)) {
}

NewObject::NewObject(const shm_handle_t & h)
   : Message(Message::NEWOBJECT, sizeof(NewObject)),
     handle(h) {

}

const shm_handle_t & NewObject::getHandle() const {

   return handle;
}

ModuleExit::ModuleExit()
   : Message(Message::MODULEEXIT, sizeof(ModuleExit)) {

}

Compute::Compute(const int m, const int c)
   : Message(Message::COMPUTE, sizeof(Compute))
   , module(m)
   , executionCount(c)
{
}

int Compute::getModule() const {

   return module;
}

int Compute::getExecutionCount() const {

   return executionCount;
}

Busy::Busy()
   : Message(Message::BUSY, sizeof(Busy)) {
}

Idle::Idle()
   : Message(Message::IDLE, sizeof(Idle)) {
}

CreateOutputPort::CreateOutputPort(const std::string & n)
   : Message(Message::CREATEOUTPUTPORT, sizeof(CreateOutputPort)) {

      COPY_STRING(name, n);
}

const char * CreateOutputPort::getName() const {

   return name;
}

CreateInputPort::CreateInputPort(const std::string & n)
   : Message(Message::CREATEINPUTPORT,
             sizeof(CreateInputPort)) {

      COPY_STRING(name, n);
}

const char * CreateInputPort::getName() const {

   return name;
}

AddObject::AddObject(const std::string & p,
                     vistle::Object::const_ptr obj)
   : Message(Message::ADDOBJECT, sizeof(AddObject)),
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

Connect::Connect(const int moduleIDA, const std::string & portA,
                 const int moduleIDB, const std::string & portB)
   : Message(Message::CONNECT, sizeof(Connect)),
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

AddParameter::AddParameter(const std::string &n, const std::string &desc, int t, int p, const std::string &mod)
: Message(Message::ADDPARAMETER, sizeof(AddParameter))
, paramtype(t)
, presentation(p) {

   assert(paramtype > Parameter::Unknown);
   assert(paramtype < Parameter::Invalid);

   assert(presentation >= Parameter::Generic);
   assert(presentation <= Parameter::InvalidPresentation);

   COPY_STRING(name, n);
   COPY_STRING(module, mod);
}

const char *AddParameter::getName() const {

   return name;
}

const char *AddParameter::moduleName() const {

   return module;
}

int AddParameter::getParameterType() const {

   return paramtype;
}

int AddParameter::getPresentation() const {

   return presentation;
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

   std::cerr << "AddParameter::getParameter: type " << type() << " not handled" << std::endl;
   assert("parameter type not supported" == 0);

   return NULL;
}

SetParameter::SetParameter(const int module,
      const std::string &n, const Parameter *param)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(param->type()) {

   COPY_STRING(name, n);
   if (const IntParameter *pint = dynamic_cast<const IntParameter *>(param)) {
      v_int = pint->getValue();
   } else if (const FloatParameter *pfloat = dynamic_cast<const FloatParameter *>(param)) {
      v_scalar = pfloat->getValue();
   } else if (const VectorParameter *pvec = dynamic_cast<const VectorParameter *>(param)) {
      ParamVector v = pvec->getValue();
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

SetParameter::SetParameter(const int module,
      const std::string &n, const int v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Integer) {

   COPY_STRING(name, n);
   v_int = v;
}

SetParameter::SetParameter(const int module,
      const std::string &n, const Scalar v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Scalar) {

   COPY_STRING(name, n);
   v_scalar = v;
}

SetParameter::SetParameter(const int module,
      const std::string &n, const ParamVector v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Vector) {

   COPY_STRING(name, n);
   dim = v.dim;
   for (int i=0; i<MaxDimension; ++i)
      v_vector[i] = v[i];
}

SetParameter::SetParameter(const int module,
      const std::string &n, const std::string &v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
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

ParamVector SetParameter::getVector() const {

   assert(paramtype == Parameter::Vector);
   return ParamVector(dim, &v_vector[0]);
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
      pvec->setValue(ParamVector(dim, &v_vector[0]));
   } else if (StringParameter *pstring = dynamic_cast<StringParameter *>(param)) {
      pstring->setValue(v_string);
   } else {
      std::cerr << "SetParameter::apply(): type " << param->type() << " not handled" << std::endl;
      assert("invalid parameter type" == 0);
   }
   
   return true;
}

Barrier::Barrier(const int id)
: Message(Message::BARRIER, sizeof(Barrier))
, barrierid(id)
{
}

int Barrier::getBarrierId() const {

   return barrierid;
}

BarrierReached::BarrierReached(const int id)
: Message(Message::BARRIERREACHED, sizeof(BarrierReached))
, barrierid(id)
{
}

int BarrierReached::getBarrierId() const {

   return barrierid;
}

} // namespace message
} // namespace vistle
