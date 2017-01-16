#include <util/tools.h>

#include "message.h"
#include "messages.h"
#include "shm.h"
#include "parameter.h"
#include "port.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

namespace vistle {
namespace message {

template<typename T>
static T min(T a, T b) { return a<b ? a : b; }

#define COPY_STRING(dst, src) \
   { \
      const size_t size = min(src.size(), dst.size()-1); \
      src.copy(dst.data(), size); \
      dst[size] = '\0'; \
      vassert(src.size() < dst.size()); \
   }

Identify::Identify(Identity id, const std::string &name)
: m_identity(id)
, m_id(Id::Invalid)
, m_rank(-1)
{
   if (id == UNKNOWN) {
       std::cerr << backtrace() << std::endl;
   }
   COPY_STRING(m_name, name);
}

Identify::Identify(Identity id, int rank)
: m_identity(id)
, m_id(Id::Invalid)
, m_rank(rank)
{
   vassert(id == Identify::LOCALBULKDATA || id == Identify::REMOTEBULKDATA);

   memset(m_name.data(), 0, m_name.size());
}

Identify::Identity Identify::identity() const {

   return m_identity;
}

const char *Identify::name() const {

   return m_name.data();
}

int Identify::id() const {

   return m_id;
}

int Identify::rank() const {

   return m_rank;
}

AddHub::AddHub(int id, const std::string &name)
: m_id(id)
, m_port(0)
, m_dataPort(0)
, m_addrType(AddHub::Unspecified)
{
   COPY_STRING(m_name, name);
   memset(m_address.data(), 0, m_address.size());
}

int AddHub::id() const {
   return m_id;
}

const char *AddHub::name() const {
   return m_name.data();
}

unsigned short AddHub::port() const {

   return m_port;
}

unsigned short AddHub::dataPort() const {

   return m_dataPort;
}

AddHub::AddressType AddHub::addressType() const {
   return m_addrType;
}

bool AddHub::hasAddress() const {
   return m_addrType == IPv6 || m_addrType == IPv4;
}

std::string AddHub::host() const {
   return m_address.data();
}

boost::asio::ip::address AddHub::address() const {
   vassert(hasAddress());
   if (addressType() == IPv6)
      return addressV6();
   else
      return addressV4();
}

boost::asio::ip::address_v6 AddHub::addressV6() const {
   vassert(m_addrType == IPv6);
   return boost::asio::ip::address_v6::from_string(m_address.data());
}

boost::asio::ip::address_v4 AddHub::addressV4() const {
   vassert(m_addrType == IPv4);
   return boost::asio::ip::address_v4::from_string(m_address.data());
}

void AddHub::setPort(unsigned short port) {
   m_port = port;
}

void AddHub::setDataPort(unsigned short port) {
   m_dataPort = port;
}

void AddHub::setAddress(boost::asio::ip::address addr) {
   vassert(addr.is_v4() || addr.is_v6());

   if (addr.is_v4())
      setAddress(addr.to_v4());
   else if (addr.is_v6())
      setAddress(addr.to_v6());
}

void AddHub::setAddress(boost::asio::ip::address_v6 addr) {
   const std::string addrString = addr.to_string();
   COPY_STRING(m_address, addrString);
   m_addrType = IPv6;
}

void AddHub::setAddress(boost::asio::ip::address_v4 addr) {
   const std::string addrString = addr.to_string();
   COPY_STRING(m_address, addrString);
   m_addrType = IPv4;
}


Ping::Ping(const char c)
: character(c)
{
}

char Ping::getCharacter() const {

   return character;
}

Pong::Pong(const Ping &ping)
: character(ping.getCharacter())
, module(ping.senderId())
{
   setReferrer(ping.uuid());
}

char Pong::getCharacter() const {

   return character;
}

int Pong::getDestination() const {

   return module;
}

Spawn::Spawn(int hub,
             const std::string & n, int mpiSize, int baseRank, int rankSkip)
   : m_hub(hub)
   , m_spawnId(Id::Invalid)
   , mpiSize(mpiSize)
   , baseRank(baseRank)
   , rankSkip(rankSkip)
{

   COPY_STRING(name, n);
}

int Spawn::hubId() const {

   return m_hub;
}

int Spawn::spawnId() const {

   return m_spawnId;
}

void Spawn::setSpawnId(int id) {

   m_spawnId = id;
}

const char * Spawn::getName() const {

   return name.data();
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

SpawnPrepared::SpawnPrepared(const Spawn &spawn)
   : m_hub(spawn.hubId())
   , m_spawnId(spawn.spawnId())
{

   setReferrer(spawn.uuid());
   COPY_STRING(name, std::string(spawn.getName()));
}

int SpawnPrepared::hubId() const {

   return m_hub;
}

int SpawnPrepared::spawnId() const {

   return m_spawnId;
}

const char * SpawnPrepared::getName() const {

   return name.data();
}

Started::Started(const std::string &n)
{

   COPY_STRING(name, n);
}

const char * Started::getName() const {

   return name.data();
}

Kill::Kill(const int m)
: module(m) {
    assert(m >= Id::ModuleBase || m == Id::Broadcast);
}

int Kill::getModule() const {

   return module;
}

Quit::Quit()
{
}

ModuleExit::ModuleExit()
: forwarded(false)
{

}

void ModuleExit::setForwarded() {

   forwarded = true;
}

bool ModuleExit::isForwarded() const {

   return forwarded;
}

Execute::Execute(Execute::What what, const int module, const int count)
   : m_allRanks(false)
   , module(module)
   , executionCount(count)
   , m_what(what)
   , m_realtime(0.)
   , m_animationStep(1./25.)
{
}

Execute::Execute(const int module, double realtime, double step)
: m_allRanks(false)
, module(module)
, executionCount(-1)
, m_what(ComputeExecute)
, m_realtime(realtime)
, m_animationStep(step)
{
}

void Execute::setModule(int m) {

   module = m;
}

int Execute::getModule() const {

   return module;
}

void Execute::setExecutionCount(int count) {

   executionCount = count;
}

int Execute::getExecutionCount() const {

   return executionCount;
}

bool Execute::allRanks() const
{
   return m_allRanks;
}

void Execute::setAllRanks(bool allRanks)
{
   m_allRanks = allRanks;
}

Execute::What Execute::what() const
{
   return m_what;
}

void Execute::setWhat(Execute::What what)
{
    m_what = what;
}

double Execute::animationRealTime() const {
    return m_realtime;
}

double Execute::animationStep() const {
    return m_animationStep;
}


Busy::Busy()
{
}

Idle::Idle()
{
}

AddPort::AddPort(const Port &port)
: m_porttype(port.getType())
, m_flags(port.flags())
{
   COPY_STRING(m_name, port.getName());
   COPY_STRING(m_description, port.getDescription());
}

Port AddPort::getPort() const {

   Port p(senderId(), m_name.data(), static_cast<Port::Type>(m_porttype), m_flags);
   p.setDescription(m_description.data());
   return p;
}

RemovePort::RemovePort(const Port &port)
{
   COPY_STRING(m_name, port.getName());
}

Port RemovePort::getPort() const {

   return Port(senderId(), m_name.data(), Port::ANY);
}

AddObject::AddObject(const std::string &sender, vistle::Object::const_ptr obj,
      const std::string & dest)
: m_name(obj->getName())
, m_meta(obj->meta())
, m_objectType(obj->getType())
, handle(obj->getHandle())
, m_handleValid(true)
{
   // we keep the handle as a reference to obj
   obj->ref();
   //std::cerr << "AddObject w/ obj: " << obj->getName() << ", refcount=" << obj->refcount() << std::endl;

   COPY_STRING(senderPort, sender);
   COPY_STRING(destPort, dest);
   COPY_STRING(m_shmname, Shm::the().name());
}

AddObject::AddObject(const AddObject &o)
: MessageBase<AddObject, ADDOBJECT>(o)
, senderPort(o.senderPort)
, destPort(o.destPort)
, m_name(o.m_name)
, m_shmname(o.m_shmname)
, m_meta(o.m_meta)
, m_objectType(o.m_objectType)
, handle(o.handle)
, m_handleValid(false)
{
    setUuid(o.uuid());
    //ref();
}

AddObject::AddObject(const AddObjectCompleted &complete)
: handle(0)
, m_handleValid(false)
{
    setUuid(complete.uuid());
    setDestId(complete.originalDestination());
}

AddObject::~AddObject() {
}

bool AddObject::ref() const {

    assert(!m_handleValid);

    if (Shm::isAttached() && Shm::the().name() == std::string(m_shmname.data())) {
        vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
        obj->ref();
        //std::cerr << "AddObject w/ AddObject: " << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
        m_handleValid = true;
    }

    return m_handleValid;
}

void AddObject::setSenderPort(const std::string &send) {
   COPY_STRING(senderPort, send);
}

const char * AddObject::getSenderPort() const {

   return senderPort.data();
}

void AddObject::setDestPort(const std::string &dest) {
   COPY_STRING(destPort, dest);
}

const char * AddObject::getDestPort() const {

   return destPort.data();
}

const char *AddObject::objectName() const {

   return m_name;
}

bool AddObject::handleValid() const {

    return m_handleValid;
}

const shm_handle_t & AddObject::getHandle() const {

   vassert(m_handleValid);
   return handle;
}

const Meta &AddObject::meta() const {
   return m_meta;
}

Object::Type AddObject::objectType() const {
   return static_cast<Object::Type>(m_objectType);
}

Object::const_ptr AddObject::takeObject() const {

   vassert(m_handleValid);
   if (Shm::isAttached() && Shm::the().name() == std::string(m_shmname.data())) {
      vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
      if (obj) {
         // ref count has been increased during AddObject construction
         obj->unref();
         //std::cerr << "takeObject: " << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
         m_handleValid = false;
      } else {
          std::cerr << "did not find " << m_name << " by handle" << std::endl;
      }
      return obj;
   }
   return getObject();
}

Object::const_ptr AddObject::getObject() const {
   auto obj = Shm::the().getObjectFromName(m_name);
   if (!obj) {
      std::cerr << "did not find " << m_name << " by name" << std::endl;
   }
   return obj;
}


AddObjectCompleted::AddObjectCompleted(const AddObject &msg)
: m_name(msg.objectName())
, m_orgDestId(msg.destId())
{
   setUuid(msg.uuid());
}

const char *AddObjectCompleted::objectName() const {

   return m_name;
}

int AddObjectCompleted::originalDestination() const {
   return m_orgDestId;
}

ObjectReceived::ObjectReceived(const AddObject &add, const std::string &p)
: m_name(add.objectName())
, m_meta(add.meta())
, m_objectType(add.objectType())
{

   m_broadcast = true;

   std::string port(add.getSenderPort());
   COPY_STRING(senderPort, port);
   COPY_STRING(portName, p);
}

const Meta &ObjectReceived::meta() const {

   return m_meta;
}

const char *ObjectReceived::getSenderPort() const {

   return senderPort.data();
}

const char *ObjectReceived::getDestPort() const {

   return portName.data();
}

const char *ObjectReceived::objectName() const {

   return m_name;
}

Object::Type ObjectReceived::objectType() const {

   return static_cast<Object::Type>(m_objectType);
}

Connect::Connect(const int moduleIDA, const std::string & portA,
                 const int moduleIDB, const std::string & portB)
   : moduleA(moduleIDA), moduleB(moduleIDB) {

        COPY_STRING(portAName, portA);
        COPY_STRING(portBName, portB);
}

const char * Connect::getPortAName() const {

   return portAName.data();
}

const char * Connect::getPortBName() const {

   return portBName.data();
}

int Connect::getModuleA() const {

   return moduleA;
}

int Connect::getModuleB() const {

   return moduleB;
}

void Connect::reverse() {

   std::swap(moduleA, moduleB);
   std::swap(portAName, portBName);
}

Disconnect::Disconnect(const int moduleIDA, const std::string & portA,
                 const int moduleIDB, const std::string & portB)
   : moduleA(moduleIDA), moduleB(moduleIDB) {

        COPY_STRING(portAName, portA);
        COPY_STRING(portBName, portB);
}

const char * Disconnect::getPortAName() const {

   return portAName.data();
}

const char * Disconnect::getPortBName() const {

   return portBName.data();
}

int Disconnect::getModuleA() const {

   return moduleA;
}

int Disconnect::getModuleB() const {

   return moduleB;
}

void Disconnect::reverse() {

   std::swap(moduleA, moduleB);
   std::swap(portAName, portBName);
}

AddParameter::AddParameter(const Parameter &param, const std::string &modname)
: paramtype(param.type())
, presentation(param.presentation())
{
   vassert(paramtype > Parameter::Unknown);
   vassert(paramtype < Parameter::Invalid);

   vassert(presentation >= Parameter::Generic);
   vassert(presentation <= Parameter::InvalidPresentation);

   COPY_STRING(name, param.getName());
   COPY_STRING(m_group, param.group());
   COPY_STRING(module, modname);
   COPY_STRING(m_description, param.description());
}

const char *AddParameter::getName() const {

   return name.data();
}

const char *AddParameter::moduleName() const {

   return module.data();
}

int AddParameter::getParameterType() const {

   return paramtype;
}

int AddParameter::getPresentation() const {

   return presentation;
}

const char *AddParameter::description() const {

   return m_description.data();
}

const char *AddParameter::group() const {

   return m_group.data();
}

std::shared_ptr<Parameter> AddParameter::getParameter() const {

   std::shared_ptr<Parameter> p;
   switch (getParameterType()) {
      case Parameter::Integer:
         p.reset(new IntParameter(senderId(), getName()));
         break;
      case Parameter::Float:
         p.reset(new FloatParameter(senderId(), getName()));
         break;
      case Parameter::Vector:
         p.reset(new VectorParameter(senderId(), getName()));
         break;
      case Parameter::IntVector:
         p.reset(new IntVectorParameter(senderId(), getName()));
         break;
      case Parameter::String:
         p.reset(new StringParameter(senderId(), getName()));
         break;
      case Parameter::Invalid:
      case Parameter::Unknown:
         break;
   }

   if (p) {

      p->setDescription(description());
      p->setGroup(group());
      p->setPresentation(Parameter::Presentation(getPresentation()));
   } else {

      std::cerr << "AddParameter::getParameter (" <<
         moduleName() << ":" << getName()
         << ": type " << getParameterType() << " not handled" << std::endl;
      vassert("parameter type not supported" == 0);
   }

   return p;
}

RemoveParameter::RemoveParameter(const Parameter &param, const std::string &modname)
: paramtype(param.type())
{
   vassert(paramtype > Parameter::Unknown);
   vassert(paramtype < Parameter::Invalid);

   COPY_STRING(name, param.getName());
   COPY_STRING(module, modname);
}

const char *RemoveParameter::getName() const {

   return name.data();
}

const char *RemoveParameter::moduleName() const {

   return module.data();
}

int RemoveParameter::getParameterType() const {

   return paramtype;
}

std::shared_ptr<Parameter> RemoveParameter::getParameter() const {

   std::shared_ptr<Parameter> p;
   switch (getParameterType()) {
      case Parameter::Integer:
         p.reset(new IntParameter(senderId(), getName()));
         break;
      case Parameter::Float:
         p.reset(new FloatParameter(senderId(), getName()));
         break;
      case Parameter::Vector:
         p.reset(new VectorParameter(senderId(), getName()));
         break;
      case Parameter::IntVector:
         p.reset(new IntVectorParameter(senderId(), getName()));
         break;
      case Parameter::String:
         p.reset(new StringParameter(senderId(), getName()));
         break;
      case Parameter::Invalid:
      case Parameter::Unknown:
         break;
   }

   return p;
}


SetParameter::SetParameter(int module, const std::string &n, const std::shared_ptr<Parameter> p, Parameter::RangeType rt)
: m_module(module)
, paramtype(p->type())
, initialize(false)
, reply(false)
, rangetype(rt)
{
   const Parameter *param = p.get();

   COPY_STRING(name, n);
   if (const auto pint = dynamic_cast<const IntParameter *>(param)) {
      v_int = pint->getValue(rt);
   } else if (const auto pfloat = dynamic_cast<const FloatParameter *>(param)) {
      v_scalar = pfloat->getValue(rt);
   } else if (const auto pvec = dynamic_cast<const VectorParameter *>(param)) {
      ParamVector v = pvec->getValue(rt);
      dim = v.dim;
      for (int i=0; i<MaxDimension; ++i)
         v_vector[i] = v[i];
   } else if (const auto ipvec = dynamic_cast<const IntVectorParameter *>(param)) {
      IntParamVector v = ipvec->getValue(rt);
      dim = v.dim;
      for (int i=0; i<MaxDimension; ++i)
         v_ivector[i] = v[i];
   } else if (const auto pstring = dynamic_cast<const StringParameter *>(param)) {
      COPY_STRING(v_string, pstring->getValue(rt));
   } else {
      std::cerr << "SetParameter: type " << param->type() << " not handled" << std::endl;
      vassert("invalid parameter type" == 0);
   }
}

SetParameter::SetParameter(int module, const std::string &n, const Integer v)
: m_module(module)
, paramtype(Parameter::Integer)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   v_int = v;
}

SetParameter::SetParameter(int module, const std::string &n, const Float v)
: m_module(module)
, paramtype(Parameter::Float)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   v_scalar = v;
}

SetParameter::SetParameter(int module, const std::string &n, const ParamVector &v)
: m_module(module)
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

SetParameter::SetParameter(int module, const std::string &n, const IntParamVector &v)
: m_module(module)
, paramtype(Parameter::IntVector)
, initialize(false)
, reply(false)
, rangetype(Parameter::Value)
{

   COPY_STRING(name, n);
   dim = v.dim;
   for (int i=0; i<MaxDimension; ++i)
      v_ivector[i] = v[i];
}

SetParameter::SetParameter(int module, const std::string &n, const std::string &v)
: m_module(module)
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

void SetParameter::setModule(int m) {

   m_module = m;
}

int SetParameter::getModule() const {

   return m_module;
}

void SetParameter::setRangeType(int rt) {

   vassert(rt >= Parameter::Minimum);
   vassert(rt <= Parameter::Maximum);
   rangetype = rt;
}

int SetParameter::rangeType() const {

   return rangetype;
}

const char *SetParameter::getName() const {

   return name.data();
}

int SetParameter::getParameterType() const {

   return paramtype;
}

Integer SetParameter::getInteger() const {

   vassert(paramtype == Parameter::Integer);
   return v_int;
}

Float SetParameter::getFloat() const {

   vassert(paramtype == Parameter::Float);
   return v_scalar;
}

ParamVector SetParameter::getVector() const {

   vassert(paramtype == Parameter::Vector);
   return ParamVector(dim, &v_vector[0]);
}

IntParamVector SetParameter::getIntVector() const {

   vassert(paramtype == Parameter::IntVector);
   return IntParamVector(dim, &v_ivector[0]);
}

std::string SetParameter::getString() const {

   vassert(paramtype == Parameter::String);
   return v_string.data();
}

bool SetParameter::apply(std::shared_ptr<vistle::Parameter> param) const {

   if (paramtype != param->type()) {
      std::cerr << "SetParameter::apply(): type mismatch for " << param->module() << ":" << param->getName() << std::endl;
      return false;
   }

   const int rt = rangeType();
   if (auto pint = std::dynamic_pointer_cast<IntParameter>(param)) {
      if (rt == Parameter::Value) pint->setValue(v_int, initialize);
      if (rt == Parameter::Minimum) pint->setMinimum(v_int);
      if (rt == Parameter::Maximum) pint->setMaximum(v_int);
   } else if (auto pfloat = std::dynamic_pointer_cast<FloatParameter>(param)) {
      if (rt == Parameter::Value) pfloat->setValue(v_scalar, initialize);
      if (rt == Parameter::Minimum) pfloat->setMinimum(v_scalar);
      if (rt == Parameter::Maximum) pfloat->setMaximum(v_scalar);
   } else if (auto pvec = std::dynamic_pointer_cast<VectorParameter>(param)) {
      if (rt == Parameter::Value) pvec->setValue(ParamVector(dim, &v_vector[0]), initialize);
      if (rt == Parameter::Minimum) pvec->setMinimum(ParamVector(dim, &v_vector[0]));
      if (rt == Parameter::Maximum) pvec->setMaximum(ParamVector(dim, &v_vector[0]));
   } else if (auto pivec = std::dynamic_pointer_cast<IntVectorParameter>(param)) {
      if (rt == Parameter::Value) pivec->setValue(IntParamVector(dim, &v_ivector[0]), initialize);
      if (rt == Parameter::Minimum) pivec->setMinimum(IntParamVector(dim, &v_ivector[0]));
      if (rt == Parameter::Maximum) pivec->setMaximum(IntParamVector(dim, &v_ivector[0]));
   } else if (auto pstring = std::dynamic_pointer_cast<StringParameter>(param)) {
      if (rt == Parameter::Value) pstring->setValue(v_string.data(), initialize);
   } else {
      std::cerr << "SetParameter::apply(): type " << param->type() << " not handled" << std::endl;
      vassert("invalid parameter type" == 0);
   }
   
   return true;
}

SetParameterChoices::SetParameterChoices(const std::string &n, const std::vector<std::string> &ch)
: numChoices(ch.size())
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

const char *SetParameterChoices::getName() const
{
   return name.data();
}

bool SetParameterChoices::apply(std::shared_ptr<vistle::Parameter> param) const {

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
      size_t len = strnlen(choices[i].data(), choices[i].size());
      ch.push_back(std::string(choices[i].data(), len));
   }

   param->setChoices(ch);
   return true;
}

Barrier::Barrier()
{
}

BarrierReached::BarrierReached(const vistle::message::uuid_t &uuid)
{
   setUuid(uuid);
}

SetId::SetId(const int id)
: m_id(id)
{
}

int SetId::getId() const {

   return m_id;
}

ReplayFinished::ReplayFinished()
{
}

SendText::SendText(const std::string &text, const Message &inResponseTo)
: m_textType(TextType::Error)
, m_referenceUuid(inResponseTo.uuid())
, m_referenceType(inResponseTo.type())
, m_truncated(false)
{
   if (text.size() >= sizeof(m_text)) {
      m_truncated = true;
   }
   COPY_STRING(m_text, text.substr(0, sizeof(m_text)-1));
}

SendText::SendText(SendText::TextType type, const std::string &text)
: m_textType(type)
, m_referenceType(INVALID)
, m_truncated(false)
{
   if (text.size() >= sizeof(m_text)) {
      m_truncated = true;
   }
   COPY_STRING(m_text, text.substr(0, sizeof(m_text)-1));
}

SendText::TextType SendText::textType() const {

   return m_textType;
}

Type SendText::referenceType() const {

   return m_referenceType;
}

uuid_t SendText::referenceUuid() const {

   return m_referenceUuid;
}

const char *SendText::text() const {

   return m_text.data();
}

bool SendText::truncated() const {

   return m_truncated;
}

ObjectReceivePolicy::ObjectReceivePolicy(ObjectReceivePolicy::Policy pol)
: m_policy(pol)
{
}

ObjectReceivePolicy::Policy ObjectReceivePolicy::policy() const {

   return m_policy;
}

SchedulingPolicy::SchedulingPolicy(SchedulingPolicy::Schedule pol)
: m_policy(pol)
{
}

SchedulingPolicy::Schedule SchedulingPolicy::policy() const {

   return m_policy;
}

ReducePolicy::ReducePolicy(ReducePolicy::Reduce red)
: m_reduce(red)
{
}

ReducePolicy::Reduce ReducePolicy::policy() const {

   return m_reduce;
}

ExecutionProgress::ExecutionProgress(Progress stage)
: m_stage(stage)
{
}

ExecutionProgress::Progress ExecutionProgress::stage() const {

   return m_stage;
}

void ExecutionProgress::setStage(ExecutionProgress::Progress stage) {

   m_stage = stage;
}

Trace::Trace(int module, Type messageType, bool onoff)
: m_module(module)
, m_messageType(messageType)
, m_on(onoff)
{
}

int Trace::module() const {

   return m_module;
}

Type Trace::messageType() const {

   return m_messageType;
}

bool Trace::on() const {

   return m_on;
}

ModuleAvailable::ModuleAvailable(int hub, const std::string &name, const std::string &path)
: m_hub(hub)
{

   COPY_STRING(m_name, name);
   COPY_STRING(m_path, path);
}

int ModuleAvailable::hub() const {

   return m_hub;
}

const char *ModuleAvailable::name() const {

    return m_name.data();
}

const char *ModuleAvailable::path() const {

   return m_path.data();
}

LockUi::LockUi(bool locked)
: m_locked(locked)
{
}

bool LockUi::locked() const {

   return m_locked;
}

RequestTunnel::RequestTunnel(unsigned short srcPort, const std::string &destHost, unsigned short destPort)
: m_srcPort(srcPort)
, m_destType(RequestTunnel::Hostname)
, m_destPort(destPort)
, m_remove(false)
{
   COPY_STRING(m_destAddr, destHost);
}

RequestTunnel::RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v6 destAddr, unsigned short destPort)
: m_srcPort(srcPort)
, m_destType(RequestTunnel::IPv6)
, m_destPort(destPort)
, m_remove(false)
{
   const std::string addr = destAddr.to_string();
   COPY_STRING(m_destAddr, addr);
}

RequestTunnel::RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v4 destAddr, unsigned short destPort)
: m_srcPort(srcPort)
, m_destType(RequestTunnel::IPv4)
, m_destPort(destPort)
, m_remove(false)
{
   const std::string addr = destAddr.to_string();
   COPY_STRING(m_destAddr, addr);
}

RequestTunnel::RequestTunnel(unsigned short srcPort, unsigned short destPort)
: m_srcPort(srcPort)
, m_destType(RequestTunnel::Unspecified)
, m_destPort(destPort)
, m_remove(false)
{
   const std::string empty;
   COPY_STRING(m_destAddr, empty);
}

RequestTunnel::RequestTunnel(unsigned short srcPort)
: m_srcPort(srcPort)
, m_destType(RequestTunnel::Unspecified)
, m_destPort(0)
, m_remove(true)
{
   const std::string empty;
   COPY_STRING(m_destAddr, empty);
}

unsigned short RequestTunnel::srcPort() const {
   return m_srcPort;
}

unsigned short RequestTunnel::destPort() const {
   return m_destPort;
}

RequestTunnel::AddressType RequestTunnel::destType() const {
   return m_destType;
}

bool RequestTunnel::destIsAddress() const {
   return m_destType == IPv6 || m_destType == IPv4;
}

void RequestTunnel::setDestAddr(boost::asio::ip::address_v4 addr) {
   const std::string addrString = addr.to_string();
   COPY_STRING(m_destAddr, addrString);
   m_destType = IPv4;
}

void RequestTunnel::setDestAddr(boost::asio::ip::address_v6 addr) {
   const std::string addrString = addr.to_string();
   COPY_STRING(m_destAddr, addrString);
   m_destType = IPv6;
}

boost::asio::ip::address RequestTunnel::destAddr() const {
   vassert(destIsAddress());
   if (destType() == IPv6)
      return destAddrV6();
   else
      return destAddrV4();
}

boost::asio::ip::address_v6 RequestTunnel::destAddrV6() const {
   vassert(m_destType == IPv6);
   return boost::asio::ip::address_v6::from_string(m_destAddr.data());
}

boost::asio::ip::address_v4 RequestTunnel::destAddrV4() const {
   vassert(m_destType == IPv4);
   return boost::asio::ip::address_v4::from_string(m_destAddr.data());
}

std::string RequestTunnel::destHost() const {
   return m_destAddr.data();
}

bool RequestTunnel::remove() const {
   return m_remove;
}

RequestObject::RequestObject(const AddObject &add, const std::string &objId, const std::string &referrer)
: m_objectId(objId)
, m_referrer(referrer.empty() ? add.objectName() : referrer)
, m_array(false)
, m_arrayType(-1)
{
   setUuid(add.uuid());
   setDestId(add.senderId());
   setDestRank(add.rank());
}

RequestObject::RequestObject(int destId, int destRank, const std::string &objId, const std::string &referrer)
: m_objectId(objId)
, m_referrer(referrer)
, m_array(false)
, m_arrayType(-1)
{
   setDestId(destId);
   setDestRank(destRank);
}

RequestObject::RequestObject(int destId, int destRank, const std::string &arrayId, int type, const std::string &referrer)
: m_objectId(arrayId)
, m_referrer(referrer)
, m_array(true)
, m_arrayType(type)
{
   setDestId(destId);
   setDestRank(destRank);
}

const char *RequestObject::objectId() const {
   return m_objectId;
}

const char *RequestObject::referrer() const {
   return m_referrer;
}

bool RequestObject::isArray() const {
   return m_array;
}

int RequestObject::arrayType() const {
    return m_arrayType;
}


SendObject::SendObject(const RequestObject &request, Object::const_ptr obj, size_t payloadSize)
: m_array(false)
, m_objectId(obj->getName())
, m_referrer(request.referrer())
, m_objectType(obj->getType())
, m_meta(obj->meta())
{
   setUuid(request.uuid());
   m_payloadSize = payloadSize;

   auto &meta = obj->meta();
   m_block = meta.block();
   m_numBlocks = meta.numBlocks();
   m_timestep = meta.timeStep();
   m_numTimesteps = meta.numTimesteps();
   m_animationstep = meta.animationStep();
   m_numAnimationsteps = meta.numAnimationSteps();
   m_iteration = meta.iteration();
   m_executionCount = meta.executionCounter();
   m_creator = meta.creator();
   m_realtime = meta.realTime();
}

SendObject::SendObject(const RequestObject &request, size_t payloadSize)
: m_array(true)
, m_objectId(request.objectId())
, m_referrer(request.referrer())
, m_objectType(request.arrayType())
{
   setUuid(request.uuid());
   m_payloadSize = payloadSize;
}

const char *SendObject::objectId() const {
   return m_objectId;
}

const char *SendObject::referrer() const {
   return m_referrer;
}

const Meta &SendObject::meta() const {
   return m_meta;
}

Object::Type SendObject::objectType() const {
   return static_cast<Object::Type>(m_objectType);
}


Meta SendObject::objectMeta() const {

   Meta meta(m_block, m_timestep, m_animationstep, m_iteration, m_executionCount, m_creator);
   meta.setNumBlocks(m_numBlocks);
   meta.setNumTimesteps(m_numTimesteps);
   meta.setNumAnimationSteps(m_numAnimationsteps);
   meta.setRealTime(m_realtime);
   return meta;
}

bool SendObject::isArray() const {
   return m_array;
}

std::ostream &operator<<(std::ostream &s, const Message &m) {

   using namespace vistle::message;

   s  << "uuid: " << boost::lexical_cast<std::string>(m.uuid())
      << ", type: " << m.type()
      << ", size: " << m.size()
      << ", sender: " << m.senderId()
      << ", dest: " << m.destId()
      << ", rank: " << m.rank()
      << ", bcast: " << m.isBroadcast();

   if (!m.referrer().is_nil())
       s << ", ref: " << boost::lexical_cast<std::string>(m.referrer());

   switch (m.type()) {
      case IDENTIFY: {
         auto &mm = static_cast<const Identify &>(m);
         s << ", identity: " << Identify::toString(mm.identity());
         break;
      }
      case EXECUTE: {
         auto &mm = static_cast<const Execute &>(m);
         s << ", module: " << mm.getModule() << ", what: " << mm.what() << ", execcount: " << mm.getExecutionCount();
         break;
      }
      case EXECUTIONPROGRESS: {
         auto &mm = static_cast<const ExecutionProgress &>(m);
         s << ", stage: " << ExecutionProgress::toString(mm.stage());
         break;
      }
      case ADDPARAMETER: {
         auto &mm = static_cast<const AddParameter &>(m);
         s << ", name: " << mm.getName();
         break;
      }
      case REMOVEPARAMETER: {
         auto &mm = static_cast<const RemoveParameter &>(m);
         s << ", name: " << mm.getName();
         break;
      }
      case SETPARAMETER: {
         auto &mm = static_cast<const SetParameter &>(m);
         s << ", name: " << mm.getName();
         break;
      }
      case SETPARAMETERCHOICES: {
         auto &mm = static_cast<const SetParameterChoices &>(m);
         s << ", name: " << mm.getName();
         break;
      }
      case ADDPORT: {
         auto &mm = static_cast<const AddPort &>(m);
         s << ", name: " << mm.getPort();
         break;
      }
      case REMOVEPORT: {
         auto &mm = static_cast<const RemovePort &>(m);
         s << ", name: " << mm.getPort();
         break;
      }
      case CONNECT: {
         auto &mm = static_cast<const Connect &>(m);
         s << ", from: " << mm.getModuleA() << ":" << mm.getPortAName() << ", to: " << mm.getModuleB() << ":" << mm.getPortBName();
         break;
      }
      case DISCONNECT: {
         auto &mm = static_cast<const Disconnect &>(m);
         s << ", from: " << mm.getModuleA() << ":" << mm.getPortAName() << ", to: " << mm.getModuleB() << ":" << mm.getPortBName();
         break;
      }
      case MODULEAVAILABLE: {
         auto &mm = static_cast<const ModuleAvailable &>(m);
         s << ", name: " << mm.name() << ", hub: " << mm.hub();
         break;
      }
      case SPAWN: {
         auto &mm = static_cast<const Spawn &>(m);
         s << ", name: " << mm.getName() << ", id: " << mm.spawnId() << ", hub: " << mm.hubId();
         break;
      }
      case ADDHUB: {
         auto &mm = static_cast<const AddHub &>(m);
         s << ", name: " << mm.name() << ", id: " << mm.id();
         break;
      }
      case SENDTEXT: {
         auto &mm = static_cast<const SendText &>(m);
         s << ", text: " << mm.text();
         break;
      }
      case ADDOBJECT: {
         auto &mm = static_cast<const AddObject &>(m);
         s << ", obj: " << mm.objectName() << ", " << mm.getSenderPort() << " -> " << mm.getDestPort() << " (handle: " << (mm.handleValid()?"valid":"invalid") << ")";
         break;
      }
      case ADDOBJECTCOMPLETED: {
         auto &mm = static_cast<const AddObjectCompleted &>(m);
         s << ", obj: " << mm.objectName() << ", original destination: " << mm.originalDestination() << std::endl;
         break;
      }
      case REQUESTOBJECT: {
         auto &mm = static_cast<const RequestObject &>(m);
         s << ", " << (mm.isArray() ? "array" : "object") << ": " << mm.objectId() << ", ref: " << mm.referrer();
         break;
      }
      case SENDOBJECT: {
         auto &mm = static_cast<const SendObject &>(m);
         s << ", " << (mm.isArray() ? "array" : "object") << ": " << mm.objectId() << ", ref: " << mm.referrer() << ", payload size: " << mm.payloadSize();
         break;
      }
      default:
         break;
   }

   return s;
}

} // namespace message
} // namespace vistle
