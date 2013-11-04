#include "message.h"
#include "shm.h"
#include "parameter.h"
#include "port.h"

#include <boost/uuid/random_generator.hpp>

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
: m_broadcast(false)
, m_uuid(boost::uuids::random_generator()())
, m_size(s)
, m_type(t)
, m_senderId(DefaultSender::id())
, m_rank(DefaultSender::rank())
{

   assert(m_size < MESSAGE_SIZE);

   assert(m_rank >= 0);
}

const Message::uuid_t &Message::uuid() const {

   return m_uuid;
}

void Message::setUuid(const Message::uuid_t &uuid) {

   m_uuid = uuid;
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

bool Message::broadcast() const {

   return m_broadcast;
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
             const std::string & n, int mpiSize, int baseRank, int rankSkip)
   : Message(Message::SPAWN, sizeof(Spawn))
   , spawnID(s)
   , mpiSize(mpiSize)
   , baseRank(baseRank)
   , rankSkip(rankSkip)
{

   COPY_STRING(name, n);
}

int Spawn::spawnId() const {

   return spawnID;
}

void Spawn::setSpawnId(int id) {

   spawnID = id;
}

const char * Spawn::getName() const {

   return name;
}

int Spawn::getMpiSize() const {

   return mpiSize;
}

int Spawn::getBaseRank() const {

   return baseRank;
}

int Spawn::getRankSkip() const {

   return rankSkip;
}

Started::Started(const std::string &n)
: Message(Message::STARTED, sizeof(Started))
{

   COPY_STRING(name, n);
}

const char * Started::getName() const {

   return name;
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

ModuleExit::ModuleExit()
: Message(Message::MODULEEXIT, sizeof(ModuleExit))
, forwarded(false)
{

}

void ModuleExit::setForwarded() {

   forwarded = true;
}

bool ModuleExit::isForwarded() const {

   return forwarded;
}

Compute::Compute(const int m, const int c)
   : Message(Message::COMPUTE, sizeof(Compute))
   , m_allRanks(false)
   , module(m)
   , executionCount(c)
{
}

void Compute::setModule(int m) {

   module = m;
}

int Compute::getModule() const {

   return module;
}

void Compute::setExecutionCount(int count) {

   executionCount = count;
}

int Compute::getExecutionCount() const {

   return executionCount;
}

bool Compute::allRanks() const
{
   return m_allRanks;
}

void Compute::setAllRanks(bool allRanks)
{
   m_allRanks = allRanks;
}


Busy::Busy()
: Message(Message::BUSY, sizeof(Busy)) {
}

Idle::Idle()
: Message(Message::IDLE, sizeof(Idle)) {
}

CreatePort::CreatePort(const Port *port)
: Message(Message::CREATEPORT, sizeof(CreatePort))
, m_porttype(port->getType())
, m_flags(port->flags())
{
   COPY_STRING(m_name, port->getName());
}

Port *CreatePort::getPort() const {

   return new Port(senderId(), m_name, static_cast<Port::Type>(m_porttype), m_flags);
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

ObjectReceived::ObjectReceived(const std::string &p,
      vistle::Object::const_ptr obj)
: Message(Message::OBJECTRECEIVED, sizeof(ObjectReceived))
, m_name(obj->getName())
, m_meta(obj->meta())
, m_objectType(obj->getType())
{

   m_broadcast = true;

   COPY_STRING(portName, p);
}

const Meta &ObjectReceived::meta() const {

   return m_meta;
}

const char *ObjectReceived::getPortName() const {

   return portName;
}

const char *ObjectReceived::objectName() const {

   return m_name;
}

Object::Type ObjectReceived::objectType() const {

   return static_cast<Object::Type>(m_objectType);
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

Disconnect::Disconnect(const int moduleIDA, const std::string & portA,
                 const int moduleIDB, const std::string & portB)
   : Message(Message::DISCONNECT, sizeof(Disconnect)),
     moduleA(moduleIDA), moduleB(moduleIDB) {

        COPY_STRING(portAName, portA);
        COPY_STRING(portBName, portB);
}

const char * Disconnect::getPortAName() const {

   return portAName;
}

const char * Disconnect::getPortBName() const {

   return portBName;
}

int Disconnect::getModuleA() const {

   return moduleA;
}

int Disconnect::getModuleB() const {

   return moduleB;
}

AddParameter::AddParameter(const Parameter *param, const std::string &modname)
: Message(Message::ADDPARAMETER, sizeof(AddParameter))
, paramtype(param->type())
, presentation(param->presentation())
{
   assert(paramtype > Parameter::Unknown);
   assert(paramtype < Parameter::Invalid);

   assert(presentation >= Parameter::Generic);
   assert(presentation <= Parameter::InvalidPresentation);

   COPY_STRING(name, param->getName());
   COPY_STRING(m_group, param->group());
   COPY_STRING(module, modname);
   COPY_STRING(m_description, param->description());
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

const char *AddParameter::description() const {

   return m_description;
}

const char *AddParameter::group() const {

   return m_group;
}

Parameter *AddParameter::getParameter() const {

   Parameter *p = nullptr;
   switch (getParameterType()) {
      case Parameter::Integer:
         p = new IntParameter(senderId(), getName());
         break;
      case Parameter::Float:
         p = new FloatParameter(senderId(), getName());
         break;
      case Parameter::Vector:
         p = new VectorParameter(senderId(), getName());
         break;
      case Parameter::String:
         p = new StringParameter(senderId(), getName());
         break;
      case Parameter::Invalid:
      case Parameter::Unknown:
         break;
   }

   if (p) {
      p->setDescription(description());
      p->setGroup(group());
      p->setPresentation(Parameter::Presentation(getPresentation()));
      return p;
   }

   std::cerr << "AddParameter::getParameter: type " << type() << " not handled" << std::endl;
   assert("parameter type not supported" == 0);

   return NULL;
}

SetParameter::SetParameter(const int module,
      const std::string &n, const Parameter *param, Parameter::RangeType rt)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(param->type())
, initialize(false)
, reply(false)
, rangetype(rt)
{

   COPY_STRING(name, n);
   if (const IntParameter *pint = dynamic_cast<const IntParameter *>(param)) {
      v_int = pint->getValue(rt);
   } else if (const FloatParameter *pfloat = dynamic_cast<const FloatParameter *>(param)) {
      v_scalar = pfloat->getValue(rt);
   } else if (const VectorParameter *pvec = dynamic_cast<const VectorParameter *>(param)) {
      ParamVector v = pvec->getValue(rt);
      dim = v.dim;
      for (int i=0; i<MaxDimension; ++i)
         v_vector[i] = v[i];
   } else if (const StringParameter *pstring = dynamic_cast<const StringParameter *>(param)) {
      COPY_STRING(v_string, pstring->getValue(rt));
   } else {
      std::cerr << "SetParameter: type " << param->type() << " not handled" << std::endl;
      assert("invalid parameter type" == 0);
   }
}

SetParameter::SetParameter(const int module,
      const std::string &n, const Integer v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Integer)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   v_int = v;
}

SetParameter::SetParameter(const int module,
      const std::string &n, const Float v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Float)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   v_scalar = v;
}

SetParameter::SetParameter(const int module,
      const std::string &n, const ParamVector v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::Vector)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   dim = v.dim;
   for (int i=0; i<MaxDimension; ++i)
      v_vector[i] = v[i];
}

SetParameter::SetParameter(const int module,
      const std::string &n, const std::string &v)
: Message(Message::SETPARAMETER, sizeof(SetParameter))
, module(module)
, paramtype(Parameter::String)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   COPY_STRING(v_string, v);
}

void SetParameter::setInit() {

   initialize = true;
}

bool SetParameter::isInitialization() const {

   return initialize;
}

void SetParameter::setReply() {

   reply = true;
}

bool SetParameter::isReply() const {

   return reply;
}

void SetParameter::setRangeType(int rt) {

   assert(rt >= Parameter::Minimum);
   assert(rt <= Parameter::Maximum);
   rangetype = rt;
}

int SetParameter::rangeType() const {

   return rangetype;
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

Integer SetParameter::getInteger() const {

   assert(paramtype == Parameter::Integer);
   return v_int;
}

Float SetParameter::getFloat() const {

   assert(paramtype == Parameter::Float);
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
      std::cerr << "SetParameter::apply(): type mismatch for " << param->module() << ":" << param->getName() << std::endl;
      return false;
   }

   const int rt = rangeType();
   if (IntParameter *pint = dynamic_cast<IntParameter *>(param)) {
      if (rt == Parameter::Value) pint->setValue(v_int, initialize);
      if (rt == Parameter::Minimum) pint->setMinimum(v_int);
      if (rt == Parameter::Maximum) pint->setMaximum(v_int);
   } else if (FloatParameter *pfloat = dynamic_cast<FloatParameter *>(param)) {
      if (rt == Parameter::Value) pfloat->setValue(v_scalar, initialize);
      if (rt == Parameter::Minimum) pfloat->setMinimum(v_scalar);
      if (rt == Parameter::Maximum) pfloat->setMaximum(v_scalar);
   } else if (VectorParameter *pvec = dynamic_cast<VectorParameter *>(param)) {
      if (rt == Parameter::Value) pvec->setValue(ParamVector(dim, &v_vector[0]), initialize);
      if (rt == Parameter::Minimum) pvec->setMinimum(ParamVector(dim, &v_vector[0]));
      if (rt == Parameter::Maximum) pvec->setMaximum(ParamVector(dim, &v_vector[0]));
   } else if (StringParameter *pstring = dynamic_cast<StringParameter *>(param)) {
      if (rt == Parameter::Value) pstring->setValue(v_string, initialize);
   } else {
      std::cerr << "SetParameter::apply(): type " << param->type() << " not handled" << std::endl;
      assert("invalid parameter type" == 0);
   }
   
   return true;
}

SetParameterChoices::SetParameterChoices(const int id, const std::string &n,
      const std::vector<std::string> &ch)
: Message(Message::SETPARAMETERCHOICES, sizeof(SetParameterChoices))
, module(id)
, numChoices(ch.size())
{
   COPY_STRING(name, n);
   if (numChoices > param_num_choices) {
      std::cerr << "SetParameterChoices::ctor: maximum number of choices (" << param_num_choices << ") exceeded (" << numChoices << ") - truncating" << std::endl;
      numChoices = param_num_choices;
   }
   for (int i=0; i<numChoices; ++i) {
      COPY_STRING(choices[i], ch[i]);
   }
}

int SetParameterChoices::getModule() const
{
   return module;
}

const char *SetParameterChoices::getName() const
{
   return name;
}

bool SetParameterChoices::apply(Parameter *param) const {

   if (param->type() != Parameter::Integer
         && param->type() != Parameter::String) {
      std::cerr << "SetParameterChoices::apply(): parameter type not compatible with choice" << std::endl;
      return false;
   }

   if (param->presentation() != Parameter::Choice) {
      std::cerr << "SetParameterChoices::apply(): parameter presentation is not 'Choice'" << std::endl;
      return false;
   }

   std::vector<std::string> ch;
   for (int i=0; i<numChoices && i<param_num_choices; ++i) {
      size_t len = strnlen(choices[i], sizeof(choices[i]));
      ch.push_back(std::string(choices[i], len));
   }

   param->setChoices(ch);
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

SetId::SetId(const int id)
: Message(Message::SETID, sizeof(SetId))
, m_id(id)
{
}

int SetId::getId() const {

   return m_id;
}

ResetModuleIds::ResetModuleIds()
: Message(Message::RESETMODULEIDS, sizeof(ResetModuleIds))
{
}

ReplayFinished::ReplayFinished()
   : Message(Message::REPLAYFINISHED, sizeof(ReplayFinished))
{
}

SendText::SendText(const std::string &text, const Message &inResponseTo)
: Message(Message::SENDTEXT, sizeof(SendText))
, m_textType(TextType::Error)
, m_referenceUuid(inResponseTo.uuid())
, m_referenceType(inResponseTo.type())
{
   COPY_STRING(m_text, text);
}

SendText::SendText(SendText::TextType type, const std::string &text)
: Message(Message::SENDTEXT, sizeof(SendText))
, m_textType(type)
, m_referenceType(Message::INVALID)
{
   COPY_STRING(m_text, text);
}

SendText::TextType SendText::textType() const {

   return m_textType;
}

Message::Type SendText::referenceType() const {

   return m_referenceType;
}

Message::uuid_t SendText::referenceUuid() const {

   return m_referenceUuid;
}

const char *SendText::text() const {

   return m_text;
}

ObjectReceivePolicy::ObjectReceivePolicy(ObjectReceivePolicy::Policy pol)
: Message(Message::OBJECTRECEIVEPOLICY, sizeof(ObjectReceivePolicy))
, m_policy(pol)
{
}

ObjectReceivePolicy::Policy ObjectReceivePolicy::policy() const {

   return m_policy;
}

SchedulingPolicy::SchedulingPolicy(SchedulingPolicy::Policy pol)
: Message(Message::SCHEDULINGPOLICY, sizeof(SchedulingPolicy))
, m_policy(pol)
{
}

SchedulingPolicy::Policy SchedulingPolicy::policy() const {

   return m_policy;
}


} // namespace message
} // namespace vistle
