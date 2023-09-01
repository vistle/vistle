#include <vistle/util/tools.h>

#include "availablemodule.h"
#include "message.h"
#include "messages.h"
#include "parameter.h"
#include "port.h"
#include "shm.h"
#include <cassert>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/archive/basic_archive.hpp>

#include "messagepayloadtemplates.h"
#include <vistle/util/vecstreambuf.h>
#include <vistle/util/crypto.h>
#include <vistle/util/tools.h>

#include <algorithm>

namespace vistle {
namespace message {

template<typename T>
static T min(T a, T b)
{
    return a < b ? a : b;
}

#define COPY_STRING(dst, src) \
    do { \
        const size_t size = min(src.size(), dst.size() - 1); \
        src.copy(dst.data(), size); \
        dst[size] = '\0'; \
        assert(src.size() < dst.size()); \
    } while (false)

template V_COREEXPORT buffer addPayload<std::string>(Message &message, const std::string &payload);
template V_COREEXPORT buffer addPayload<SendText::Payload>(Message &message, const SendText::Payload &payload);
template V_COREEXPORT buffer addPayload<ItemInfo::Payload>(Message &message, const ItemInfo::Payload &payload);
template V_COREEXPORT buffer addPayload<SetParameterChoices::Payload>(Message &message,
                                                                      const SetParameterChoices::Payload &payload);


template V_COREEXPORT std::string getPayload(const buffer &data);
template V_COREEXPORT SendText::Payload getPayload(const buffer &data);
template V_COREEXPORT ItemInfo::Payload getPayload(const buffer &data);
template V_COREEXPORT SetParameterChoices::Payload getPayload(const buffer &data);

Identify::Identify(const std::string &name)
: m_identity(Identity::REQUEST)
, m_numRanks(-1)
, m_rank(-1)
, m_boost_archive_version(boost::archive::BOOST_ARCHIVE_VERSION())
, m_indexSize(sizeof(vistle::Index))
, m_scalarSize(sizeof(vistle::Scalar))
{
    COPY_STRING(m_name, name);

    auto &sd = crypto::session_data();
    assert(sd.size() == m_session_data.size());
    memcpy(m_session_data.data(), sd.data(), std::min(m_session_data.size(), sd.size()));
}

Identify::Identify(const Identify &request, Identify::Identity id, const std::string &name)
: m_identity(id)
, m_numRanks(-1)
, m_rank(-1)
, m_boost_archive_version(boost::archive::BOOST_ARCHIVE_VERSION())
, m_indexSize(sizeof(vistle::Index))
, m_scalarSize(sizeof(vistle::Scalar))
{
    setReferrer(request.uuid());
    m_session_data = request.m_session_data;

    if (id == UNKNOWN || id == REQUEST) {
        std::cerr << "invalid identity " << toString(id) << std::endl;
        std::cerr << backtrace() << std::endl;
    }
    COPY_STRING(m_name, name);

    computeMac();
}

Identify::Identify(const Identify &request, Identity id, int rank)
: m_identity(id)
, m_numRanks(-1)
, m_rank(rank)
, m_boost_archive_version(boost::archive::BOOST_ARCHIVE_VERSION())
, m_indexSize(sizeof(vistle::Index))
, m_scalarSize(sizeof(vistle::Scalar))
{
    setReferrer(request.uuid());
    m_session_data = request.m_session_data;

    assert(id == Identify::LOCALBULKDATA || id == Identify::REMOTEBULKDATA);

    memset(m_name.data(), 0, m_name.size());

    computeMac();
}

Identify::Identity Identify::identity() const
{
    return m_identity;
}

const char *Identify::name() const
{
    return m_name.data();
}

int Identify::rank() const
{
    return m_rank;
}

int Identify::numRanks() const
{
    return m_numRanks;
}

size_t Identify::pid() const
{
    return m_pid;
}

void Identify::setPid(size_t pid)
{
    m_pid = pid;
}

int Identify::boost_archive_version() const
{
    return m_boost_archive_version;
}

int Identify::indexSize() const
{
    return m_indexSize;
}

int Identify::scalarSize() const
{
    return m_scalarSize;
}

void Identify::setNumRanks(int size)
{
    m_numRanks = size;
}

void Identify::computeMac()
{
    auto key = crypto::session_key();
    std::fill(m_mac.begin(), m_mac.end(), 0);

    auto mac = crypto::compute_mac(static_cast<const void *>(this), sizeof(*this), key);
    assert(m_mac.size() == mac.size());
    memcpy(m_mac.data(), mac.data(), m_mac.size());

    assert(verifyMac(false));
}

bool Identify::verifyMac(bool compareSessionData) const
{
    if (compareSessionData) {
        auto s = crypto::session_data();
        if (memcmp(s.data(), m_session_data.data(), std::min(s.size(), m_session_data.size())) != 0) {
            std::vector<uint8_t> session_data(m_session_data.begin(), m_session_data.end());
            std::cerr << "Identify::verifyMac: session data comparison failed: size is " << m_session_data.size()
                      << ", expected size " << s.size() << ", data is '" << crypto::hex_encode(session_data)
                      << "', expected '" << crypto::hex_encode(s) << "'" << std::endl;
            return false;
        }
    }

    auto key = crypto::session_key();
    message::Buffer buf(*this);
    auto &id = buf.as<Identify>();
    std::fill(id.m_mac.begin(), id.m_mac.end(), 0);

    std::vector<uint8_t> mac(m_mac.size());
    memcpy(mac.data(), m_mac.data(), mac.size());
    if (!crypto::verify_mac(static_cast<const void *>(buf.data()), sizeof(*this), key, mac)) {
        std::cerr << "Identify::verifyMac: verification with Botan failed" << std::endl;
        return false;
    }
    return true;
}

AddHub::AddHub(int id, const std::string &name)
: m_id(id), m_port(0), m_dataPort(0), m_addrType(AddHub::Unspecified), m_hasUserInterface(false), m_hasVrb(false)
{
    COPY_STRING(m_name, name);
    memset(m_loginName.data(), 0, m_loginName.size());
    memset(m_realName.data(), 0, m_realName.size());
    memset(m_address.data(), 0, m_address.size());
}

int AddHub::id() const
{
    return m_id;
}

const char *AddHub::name() const
{
    return m_name.data();
}

const char *AddHub::loginName() const
{
    return m_loginName.data();
}

void AddHub::setLoginName(const std::string &login)
{
    COPY_STRING(m_loginName, login);
}

const char *AddHub::realName() const
{
    return m_realName.data();
}

void AddHub::setRealName(const std::string &real)
{
    COPY_STRING(m_realName, real);
}

int AddHub::numRanks() const
{
    return m_numRanks;
}

unsigned short AddHub::port() const
{
    return m_port;
}

unsigned short AddHub::dataPort() const
{
    return m_dataPort;
}

AddHub::AddressType AddHub::addressType() const
{
    return m_addrType;
}

bool AddHub::hasAddress() const
{
    return m_addrType == IPv6 || m_addrType == IPv4;
}

std::string AddHub::host() const
{
    return m_address.data();
}

boost::asio::ip::address AddHub::address() const
{
    assert(hasAddress());
    if (addressType() == IPv6)
        return addressV6();
    else
        return addressV4();
}

boost::asio::ip::address_v6 AddHub::addressV6() const
{
    assert(m_addrType == IPv6);
    return boost::asio::ip::address_v6::from_string(m_address.data());
}

boost::asio::ip::address_v4 AddHub::addressV4() const
{
    assert(m_addrType == IPv4);
    return boost::asio::ip::address_v4::from_string(m_address.data());
}

void AddHub::setNumRanks(int size)
{
    m_numRanks = size;
}

void AddHub::setPort(unsigned short port)
{
    m_port = port;
}

void AddHub::setDataPort(unsigned short port)
{
    m_dataPort = port;
}

void AddHub::setAddress(boost::asio::ip::address addr)
{
    assert(addr.is_v4() || addr.is_v6());

    if (addr.is_v4())
        setAddress(addr.to_v4());
    else if (addr.is_v6())
        setAddress(addr.to_v6());
}

void AddHub::setAddress(boost::asio::ip::address_v6 addr)
{
    const std::string addrString = addr.to_string();
    COPY_STRING(m_address, addrString);
    m_addrType = IPv6;
}

void AddHub::setAddress(boost::asio::ip::address_v4 addr)
{
    const std::string addrString = addr.to_string();
    COPY_STRING(m_address, addrString);
    m_addrType = IPv4;
}

void AddHub::setHasUserInterface(bool ui)
{
    m_hasUserInterface = ui;
}

bool AddHub::hasUserInterface() const
{
    return m_hasUserInterface;
}

void AddHub::setHasVrb(bool ui)
{
    m_hasVrb = ui;
}

bool AddHub::hasVrb() const
{
    return m_hasVrb;
}

std::string AddHub::systemType() const
{
    return m_systemType.data();
}

void AddHub::setSystemType(const std::string &systemType)
{
    COPY_STRING(m_systemType, systemType);
}

std::string AddHub::arch() const
{
    return m_arch.data();
}

void AddHub::setArch(const std::string &arch)
{
    COPY_STRING(m_arch, arch);
}


RemoveHub::RemoveHub(int id): m_id(id)
{}

int RemoveHub::id() const
{
    return m_id;
}


LoadWorkflow::LoadWorkflow(const std::string &pathname)
{
    COPY_STRING(m_pathname, pathname);
}

const char *LoadWorkflow::pathname() const
{
    return m_pathname.data();
}


SaveWorkflow::SaveWorkflow(const std::string &pathname)
{
    COPY_STRING(m_pathname, pathname);
}

const char *SaveWorkflow::pathname() const
{
    return m_pathname.data();
}


Spawn::Spawn(int hub, const std::string &n, int mpiSize, int baseRank, int rankSkip)
: m_hub(hub), m_spawnId(Id::Invalid), mpiSize(mpiSize), baseRank(baseRank), rankSkip(rankSkip)
{
    COPY_STRING(name, n);
}

void Spawn::setReference(int id, ReferenceType type)
{
    m_referenceId = id;
    m_referenceType = (int)type;
}

int Spawn::getReference() const
{
    return m_referenceId;
}

Spawn::ReferenceType Spawn::getReferenceType() const
{
    return (ReferenceType)m_referenceType;
}

void Spawn::setMigrateId(int id)
{
    setReference(id, ReferenceType::Migrate);
}

int Spawn::migrateId() const
{
    return m_referenceType == (int)ReferenceType::Migrate ? m_referenceId : Id::Invalid;
}

void Spawn::setMirroringId(int id)
{
    setReference(id, ReferenceType::Mirror);
}

int Spawn::mirroringId() const
{
    return m_referenceType == (int)ReferenceType::Mirror ? m_referenceId : Id::Invalid;
}

int Spawn::hubId() const
{
    return m_hub;
}

void Spawn::setHubId(int hub)
{
    m_hub = hub;
}

int Spawn::spawnId() const
{
    return m_spawnId;
}

void Spawn::setSpawnId(int id)
{
    m_spawnId = id;
}

const char *Spawn::getName() const
{
    return name.data();
}

void Spawn::setName(const char *mod)
{
    assert(mod);
    COPY_STRING(name, std::string(mod));
}

int Spawn::getMpiSize() const
{
    return mpiSize;
}

int Spawn::getBaseRank() const
{
    return baseRank;
}

int Spawn::getRankSkip() const
{
    return rankSkip;
}

SpawnPrepared::SpawnPrepared(const Spawn &spawn): m_hub(spawn.hubId()), m_spawnId(spawn.spawnId())
{
    setReferrer(spawn.uuid());
    COPY_STRING(name, std::string(spawn.getName()));
}

int SpawnPrepared::hubId() const
{
    return m_hub;
}

int SpawnPrepared::spawnId() const
{
    return m_spawnId;
}

const char *SpawnPrepared::getName() const
{
    return name.data();
}

Started::Started(const std::string &n)
{
    COPY_STRING(name, n);
}

const char *Started::getName() const
{
    return name.data();
}

size_t Started::pid() const
{
    return m_pid;
}

void Started::setPid(size_t pid)
{
    m_pid = pid;
}

Kill::Kill(const int m): module(m)
{
    assert(Id::isModule(m) || Id::isHub(m) || m == Id::Broadcast);
}

int Kill::getModule() const
{
    return module;
}

Debug::Debug(const int m, Debug::Request req): m_module(m), m_request(req)
{
    if (req == AttachDebugger) {
        assert(message::Id::isHub(m) || message::Id::isModule(m));
    }
}

int Debug::getModule() const
{
    return m_module;
}

Debug::Request Debug::getRequest() const
{
    return static_cast<Request>(m_request);
}


Quit::Quit(int id): m_id(id)
{}

int Quit::id() const
{
    return m_id;
}


CloseConnection::CloseConnection(const std::string &reason)
{
    COPY_STRING(m_reason, reason);
}

const char *CloseConnection::reason() const
{
    return m_reason.data();
}

ModuleExit::ModuleExit(): forwarded(false)
{}

void ModuleExit::setForwarded()
{
    forwarded = true;
}

bool ModuleExit::isForwarded() const
{
    return forwarded;
}

Screenshot::Screenshot(const std::string &filename, bool quit): m_quit(quit)
{
    COPY_STRING(m_filename, filename);
}

const char *Screenshot::filename() const
{
    return m_filename.data();
}

bool Screenshot::quit() const
{
    return m_quit;
}

Execute::Execute(Execute::What what, const int module, const int count)
: m_allRanks(false), module(module), executionCount(count), m_what(what), m_realtime(0.), m_animationStepDuration(0.)
{}

Execute::Execute(const int module, double realtime, double stepsize)
: m_allRanks(false)
, module(module)
, executionCount(-1)
, m_what(ComputeExecute)
, m_realtime(realtime)
, m_animationStepDuration(stepsize)
{}

void Execute::setModule(int m)
{
    module = m;
}

int Execute::getModule() const
{
    return module;
}

void Execute::setExecutionCount(int count)
{
    executionCount = count;
}

int Execute::getExecutionCount() const
{
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

double Execute::animationRealTime() const
{
    return m_realtime;
}

double Execute::animationStepDuration() const
{
    return m_animationStepDuration;
}

ExecutionDone::ExecutionDone(int executionCount): m_executionCount(executionCount)
{}

int ExecutionDone::getExecutionCount() const
{
    return m_executionCount;
}


CancelExecute::CancelExecute(const int module): m_module(module)
{}

int CancelExecute::getModule() const
{
    return m_module;
}


Busy::Busy()
{}

Idle::Idle()
{}

AddPort::AddPort(const Port &port): m_porttype(port.getType()), m_flags(port.flags())
{
    COPY_STRING(m_name, port.getName());
    COPY_STRING(m_description, port.getDescription());
}

Port AddPort::getPort() const
{
    Port p(senderId(), m_name.data(), static_cast<Port::Type>(m_porttype), m_flags);
    p.setDescription(m_description.data());
    return p;
}

RemovePort::RemovePort(const Port &port)
{
    COPY_STRING(m_name, port.getName());
}

Port RemovePort::getPort() const
{
    return Port(senderId(), m_name.data(), Port::ANY);
}

AddObject::AddObject(const std::string &sender, vistle::Object::const_ptr obj, const std::string &dest)
: m_meta(obj->meta())
, m_objectType(obj->getType())
, m_name(obj->getName())
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
, m_meta(o.m_meta)
, m_objectType(o.m_objectType)
, senderPort(o.senderPort)
, destPort(o.destPort)
, m_name(o.m_name)
, m_shmname(o.m_shmname)
, handle(o.handle)
, m_handleValid(false)
{
    setUuid(o.uuid());
    //ref();
}

AddObject::AddObject(const AddObjectCompleted &complete): m_objectType(Object::UNKNOWN), handle(0), m_handleValid(false)
{
    setUuid(complete.referrer());
    setDestId(complete.originalDestination());
    setDestPort(complete.originalDestPort());
}

AddObject::~AddObject() = default;

bool AddObject::operator<(const AddObject &other) const
{
    if (uuid() == other.uuid()) {
        if (destId() == other.destId()) {
            return strcmp(getDestPort(), other.getDestPort()) < 0;
        }
        return destId() < other.destId();
    }
    return uuid() < other.uuid();
}

bool AddObject::ref() const
{
    assert(!m_handleValid);

    if (Shm::isAttached() && Shm::the().name() == std::string(m_shmname.data())) {
        vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
        obj->ref();
        //std::cerr << "AddObject w/ AddObject: " << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
        m_handleValid = true;
    }

    return m_handleValid;
}

void AddObject::setSenderPort(const std::string &send)
{
    COPY_STRING(senderPort, send);
}

const char *AddObject::getSenderPort() const
{
    return senderPort.data();
}

void AddObject::setDestPort(const std::string &dest)
{
    COPY_STRING(destPort, dest);
}

const char *AddObject::getDestPort() const
{
    return destPort.data();
}

const char *AddObject::objectName() const
{
    return m_name;
}

bool AddObject::handleValid() const
{
    return m_handleValid;
}

const shm_handle_t &AddObject::getHandle() const
{
    assert(m_handleValid);
    return handle;
}

const Meta &AddObject::meta() const
{
    return m_meta;
}

Object::Type AddObject::objectType() const
{
    return static_cast<Object::Type>(m_objectType);
}

void AddObject::setObject(Object::const_ptr obj)
{
    if (m_handleValid) {
        if (Shm::isAttached() && Shm::the().name() == std::string(m_shmname.data())) {
            vistle::Object::const_ptr o = Shm::the().getObjectFromHandle(handle);
            assert(o);
            o->unref();
        }

        m_handleValid = false;
    }

    COPY_STRING(m_shmname, Shm::the().name());
    if (obj) {
        handle = obj->getHandle();
        obj->ref();
    } else {
        handle = 0;
    }
    m_handleValid = true;
}

Object::const_ptr AddObject::takeObject() const
{
    assert(m_handleValid);
    if (Shm::isAttached() && Shm::the().name() == std::string(m_shmname.data())) {
        vistle::Object::const_ptr obj = Shm::the().getObjectFromHandle(handle);
        if (obj) {
            assert(obj->getName() == objectName());
            // ref count has been increased during AddObject construction
            obj->unref();
            //std::cerr << "takeObject: " << obj->getName() << ", refcount=" << obj->refcount() << std::endl;
            m_handleValid = false;
        } else {
            std::cerr << "AddObject::takeObject: did not find " << m_name << " by handle" << std::endl;
        }
        return obj;
    }
    return getObject();
}

Object::const_ptr AddObject::getObject() const
{
    auto obj = Shm::the().getObjectFromName(m_name);
    if (!obj) {
        //std::cerr << "AddObject::getObject: did not find " << m_name << " by name" << std::endl;
    }
    return obj;
}

void AddObject::setBlocker()
{
    m_blocker = true;
}

bool AddObject::isBlocker() const
{
    return m_blocker;
}

void AddObject::setUnblocking()
{
    m_unblock = true;
}

bool AddObject::isUnblocking() const
{
    return m_unblock;
}


AddObjectCompleted::AddObjectCompleted(const AddObject &msg): m_name(msg.objectName()), m_orgDestId(msg.destId())
{
    std::string port(msg.getDestPort());
    COPY_STRING(m_orgDestPort, port);
    setReferrer(msg.uuid());

    setDestId(msg.senderId());
    setDestRank(msg.rank());
}

const char *AddObjectCompleted::objectName() const
{
    return m_name;
}

int AddObjectCompleted::originalDestination() const
{
    return m_orgDestId;
}

const char *AddObjectCompleted::originalDestPort() const
{
    return m_orgDestPort.data();
}

template<Type MessageType>
ConnectBase<MessageType>::ConnectBase(const int moduleIDA, const std::string &portA, const int moduleIDB,
                                      const std::string &portB)
: moduleA(moduleIDA), moduleB(moduleIDB)
{
    COPY_STRING(portAName, portA);
    COPY_STRING(portBName, portB);
}

template<Type MessageType>
const char *ConnectBase<MessageType>::getPortAName() const
{
    return portAName.data();
}

template<Type MessageType>
const char *ConnectBase<MessageType>::getPortBName() const
{
    return portBName.data();
}

template<Type MessageType>
void ConnectBase<MessageType>::setModuleA(int id)
{
    moduleA = id;
}

template<Type MessageType>
int ConnectBase<MessageType>::getModuleA() const
{
    return moduleA;
}

template<Type MessageType>
void ConnectBase<MessageType>::setModuleB(int id)
{
    moduleB = id;
}

template<Type MessageType>
int ConnectBase<MessageType>::getModuleB() const
{
    return moduleB;
}

template<Type MessageType>
void ConnectBase<MessageType>::reverse()
{
    std::swap(moduleA, moduleB);
    std::swap(portAName, portBName);
}

template class ConnectBase<CONNECT>;
template class ConnectBase<DISCONNECT>;

AddParameter::AddParameter(const Parameter &param, const std::string &modname)
: paramtype(param.type()), presentation(param.presentation()), m_groupExpanded(param.isGroupExpanded())
{
    assert(paramtype > Parameter::Unknown);
    assert(paramtype < Parameter::Invalid);

    assert(presentation >= Parameter::Generic);
    assert(presentation <= Parameter::InvalidPresentation);

    COPY_STRING(name, param.getName());
    COPY_STRING(m_group, param.group());
    COPY_STRING(module, modname);
    COPY_STRING(m_description, param.description());
}

const char *AddParameter::getName() const
{
    return name.data();
}

const char *AddParameter::moduleName() const
{
    return module.data();
}

Parameter::Type AddParameter::getParameterType() const
{
    return static_cast<Parameter::Type>(paramtype);
}

Parameter::Presentation AddParameter::getPresentation() const
{
    return static_cast<Parameter::Presentation>(presentation);
}

const char *AddParameter::description() const
{
    return m_description.data();
}

const char *AddParameter::group() const
{
    return m_group.data();
}

bool AddParameter::isGroupExpanded() const
{
    return m_groupExpanded;
}

std::shared_ptr<Parameter> AddParameter::getParameter() const
{
    std::shared_ptr<Parameter> p = vistle::getParameter(senderId(), getName(), Parameter::Type(getParameterType()),
                                                        Parameter::Presentation(getPresentation()));
    if (p) {
        p->setDescription(description());
        p->setGroup(group());
        p->setGroupExpanded(isGroupExpanded());
    } else {
        std::cerr << "AddParameter::getParameter: " << moduleName() << ":" << getName() << ": type "
                  << getParameterType() << " not handled" << std::endl;
        assert("parameter type not supported" == 0);
    }

    return p;
}

RemoveParameter::RemoveParameter(const Parameter &param, const std::string &modname): paramtype(param.type())
{
    assert(paramtype > Parameter::Unknown);
    assert(paramtype < Parameter::Invalid);

    COPY_STRING(name, param.getName());
    COPY_STRING(module, modname);
}

const char *RemoveParameter::getName() const
{
    return name.data();
}

const char *RemoveParameter::moduleName() const
{
    return module.data();
}

Parameter::Type RemoveParameter::getParameterType() const
{
    return static_cast<Parameter::Type>(paramtype);
}

std::shared_ptr<Parameter> RemoveParameter::getParameter() const
{
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
    case Parameter::StringVector:
        p.reset(new StringVectorParameter(senderId(), getName()));
        break;
    case Parameter::Invalid:
    case Parameter::Unknown:
        break;
    }

    return p;
}


SetParameter::SetParameter(int module)
: m_module(module)
, paramtype(Parameter::Invalid)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    name[0] = '\0';
}

SetParameter::SetParameter(int module, const std::string &n, const std::shared_ptr<Parameter> p,
                           Parameter::RangeType rt, bool defaultValue)
: m_module(module)
, paramtype(p->type())
, rangetype(rt)
, initialize(defaultValue)
, reply(false)
, delayed(false)
, read_only_valid(true)
, read_only(p->isReadOnly())
, immediate_valid(true)
, immediate(p->isImmediate())
{
    const Parameter *param = p.get();

    assert(!initialize || rt == Parameter::Value);

    COPY_STRING(name, n);
    if (const auto pint = dynamic_cast<const IntParameter *>(param)) {
        v_int = initialize ? pint->getDefaultValue() : pint->getValue(rt);
    } else if (const auto pfloat = dynamic_cast<const FloatParameter *>(param)) {
        v_scalar = initialize ? pfloat->getDefaultValue() : pfloat->getValue(rt);
    } else if (const auto pvec = dynamic_cast<const VectorParameter *>(param)) {
        ParamVector v = initialize ? pvec->getDefaultValue() : pvec->getValue(rt);
        dim = v.dim;
        for (int i = 0; i < MaxDimension; ++i)
            v_vector[i] = v[i];
    } else if (const auto ipvec = dynamic_cast<const IntVectorParameter *>(param)) {
        IntParamVector v = initialize ? ipvec->getDefaultValue() : ipvec->getValue(rt);
        dim = v.dim;
        for (int i = 0; i < MaxDimension; ++i)
            v_ivector[i] = v[i];
    } else if (const auto pstring = dynamic_cast<const StringParameter *>(param)) {
        COPY_STRING(v_string, (initialize ? pstring->getDefaultValue() : pstring->getValue(rt)));
    } else if (const auto spvec = dynamic_cast<const StringVectorParameter *>(param)) {
        StringParamVector v = initialize ? spvec->getDefaultValue() : spvec->getValue(rt);
        dim = v.dim;
        for (int i = 0; i < MaxDimension; ++i) {
            //FIXME
            //v_svector[i] = v[i];
        }
    } else {
        std::cerr << "SetParameter: type " << param->type() << " not handled" << std::endl;
        assert("invalid parameter type" == 0);
    }
}

SetParameter::SetParameter(int module, const std::string &n, const Integer v)
: m_module(module)
, paramtype(Parameter::Integer)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    COPY_STRING(name, n);
    v_int = v;
}

SetParameter::SetParameter(int module, const std::string &n, const Float v)
: m_module(module)
, paramtype(Parameter::Float)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    COPY_STRING(name, n);
    v_scalar = v;
}

SetParameter::SetParameter(int module, const std::string &n, const ParamVector &v)
: m_module(module)
, paramtype(Parameter::Vector)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    COPY_STRING(name, n);
    dim = v.dim;
    for (int i = 0; i < MaxDimension; ++i)
        v_vector[i] = v[i];
}

SetParameter::SetParameter(int module, const std::string &n, const IntParamVector &v)
: m_module(module)
, paramtype(Parameter::IntVector)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    COPY_STRING(name, n);
    dim = v.dim;
    for (int i = 0; i < MaxDimension; ++i)
        v_ivector[i] = v[i];
}

SetParameter::SetParameter(int module, const std::string &n, const std::string &v)
: m_module(module)
, paramtype(Parameter::String)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    COPY_STRING(name, n);
    COPY_STRING(v_string, v);
}

SetParameter::SetParameter(int module, const std::string &n, const StringParamVector &v)
: m_module(module)
, paramtype(Parameter::String)
, rangetype(Parameter::Value)
, initialize(false)
, reply(false)
, delayed(false)
, immediate_valid(false)
, immediate(false)
{
    COPY_STRING(name, n);
    //FIXME
    //COPY_STRING(v_string, v);
}

void SetParameter::setInit()
{
    initialize = true;
}

bool SetParameter::isInitialization() const
{
    return initialize;
}

void SetParameter::setDelayed()
{
    delayed = true;
}

bool SetParameter::isDelayed() const
{
    return delayed;
}

void SetParameter::setModule(int m)
{
    m_module = m;
}

int SetParameter::getModule() const
{
    return m_module;
}

void SetParameter::setRangeType(int rt)
{
    assert(rt >= Parameter::Minimum);
    assert(rt <= Parameter::Maximum);
    rangetype = rt;
}

int SetParameter::rangeType() const
{
    return rangetype;
}

const char *SetParameter::getName() const
{
    return name.data();
}

Parameter::Type SetParameter::getParameterType() const
{
    return static_cast<Parameter::Type>(paramtype);
}

Integer SetParameter::getInteger() const
{
    assert(paramtype == Parameter::Integer);
    return v_int;
}

Float SetParameter::getFloat() const
{
    assert(paramtype == Parameter::Float);
    return v_scalar;
}

ParamVector SetParameter::getVector() const
{
    assert(paramtype == Parameter::Vector);
    return ParamVector(dim, &v_vector[0]);
}

IntParamVector SetParameter::getIntVector() const
{
    assert(paramtype == Parameter::IntVector);
    return IntParamVector(dim, &v_ivector[0]);
}

std::string SetParameter::getString() const
{
    assert(paramtype == Parameter::String);
    return v_string.data();
}

StringParamVector SetParameter::getStringVector() const
{
    assert(paramtype == Parameter::StringVector);
    //FIXME
    return StringParamVector();
}

void SetParameter::setReadOnly(bool readOnly)
{
    read_only_valid = true;
    read_only = readOnly;
}

bool SetParameter::isReadOnly() const
{
    return read_only;
}

void SetParameter::setImmediate(bool immed)
{
    immediate_valid = true;
    immediate = immed;
}

bool SetParameter::isImmediate() const
{
    return immediate;
}

bool SetParameter::apply(std::shared_ptr<vistle::Parameter> param) const
{
    if (paramtype != param->type()) {
        std::cerr << "SetParameter::apply(): type mismatch for " << param->module() << ":" << param->getName()
                  << std::endl;
        return false;
    }

    if (immediate_valid)
        param->setImmediate(immediate);
    if (read_only_valid)
        param->setReadOnly(read_only);

    const int rt = rangeType();
    if (rt == Parameter::Other)
        return true;

    if (auto pint = std::dynamic_pointer_cast<IntParameter>(param)) {
        if (rt == Parameter::Value)
            pint->setValue(v_int, initialize, delayed);
        if (rt == Parameter::Minimum)
            pint->setMinimum(v_int);
        if (rt == Parameter::Maximum)
            pint->setMaximum(v_int);
    } else if (auto pfloat = std::dynamic_pointer_cast<FloatParameter>(param)) {
        if (rt == Parameter::Value)
            pfloat->setValue(v_scalar, initialize, delayed);
        if (rt == Parameter::Minimum)
            pfloat->setMinimum(v_scalar);
        if (rt == Parameter::Maximum)
            pfloat->setMaximum(v_scalar);
    } else if (auto pvec = std::dynamic_pointer_cast<VectorParameter>(param)) {
        if (rt == Parameter::Value)
            pvec->setValue(ParamVector(dim, &v_vector[0]), initialize, delayed);
        if (rt == Parameter::Minimum)
            pvec->setMinimum(ParamVector(dim, &v_vector[0]));
        if (rt == Parameter::Maximum)
            pvec->setMaximum(ParamVector(dim, &v_vector[0]));
    } else if (auto pivec = std::dynamic_pointer_cast<IntVectorParameter>(param)) {
        if (rt == Parameter::Value)
            pivec->setValue(IntParamVector(dim, &v_ivector[0]), initialize);
        if (rt == Parameter::Minimum)
            pivec->setMinimum(IntParamVector(dim, &v_ivector[0]));
        if (rt == Parameter::Maximum)
            pivec->setMaximum(IntParamVector(dim, &v_ivector[0]));
    } else if (auto pstring = std::dynamic_pointer_cast<StringParameter>(param)) {
        if (rt == Parameter::Value)
            pstring->setValue(v_string.data(), initialize, delayed);
        if (rt == Parameter::Minimum)
            pstring->setMinimum(v_string.data());
        if (rt == Parameter::Maximum)
            pstring->setMaximum(v_string.data());
    } else {
        std::cerr << "SetParameter::apply(): type " << param->type() << " not handled" << std::endl;
        assert("invalid parameter type" == 0);
    }

    return true;
}

SetParameterChoices::SetParameterChoices(const std::string &n, unsigned numChoices): numChoices(numChoices)
{
    COPY_STRING(name, n);
}

const char *SetParameterChoices::getName() const
{
    return name.data();
}

bool SetParameterChoices::apply(std::shared_ptr<vistle::Parameter> param,
                                const SetParameterChoices::Payload &payload) const
{
    if (param->type() != Parameter::Integer && param->type() != Parameter::String) {
        std::cerr << "SetParameterChoices::apply(): " << param->module() << ":" << param->getName()
                  << ": type not compatible with choice" << std::endl;
        return false;
    }

    if (param->presentation() != Parameter::Choice) {
        std::cerr << "SetParameterChoices::apply(): " << param->module() << ":" << param->getName()
                  << ": parameter presentation is not 'Choice'" << std::endl;
        return false;
    }

    param->setChoices(payload.choices);
    return true;
}

Barrier::Barrier()
{}

BarrierReached::BarrierReached(const vistle::message::uuid_t &uuid)
{
    setUuid(uuid);
}

SetId::SetId(const int id): m_id(id)
{}

int SetId::getId() const
{
    return m_id;
}

ReplayFinished::ReplayFinished()
{}

SendText::SendText(const Message &inResponseTo)
: m_textType(TextType::Error), m_referenceUuid(inResponseTo.uuid()), m_referenceType(inResponseTo.type())
{}

SendText::SendText(SendText::TextType type): m_textType(type), m_referenceType(INVALID)
{}

SendText::TextType SendText::textType() const
{
    return m_textType;
}

Type SendText::referenceType() const
{
    return m_referenceType;
}

uuid_t SendText::referenceUuid() const
{
    return m_referenceUuid;
}

SendText::Payload::Payload() = default;

SendText::Payload::Payload(const std::string &text): text(text)
{}

ItemInfo::ItemInfo(ItemInfo::InfoType type, const std::string port): m_infoType(type)
{
    COPY_STRING(m_port, port);
}

ItemInfo::InfoType ItemInfo::infoType() const
{
    return m_infoType;
}

const char *ItemInfo::port() const
{
    return m_port.data();
}

ItemInfo::Payload::Payload() = default;

ItemInfo::Payload::Payload(const std::string &text): text(text)
{}

UpdateStatus::UpdateStatus(const std::string &text, UpdateStatus::Importance prio)
: m_importance(prio), m_statusType(UpdateStatus::Text)
{
    COPY_STRING(m_text, text.substr(0, sizeof(m_text) - 1));
}

UpdateStatus::UpdateStatus(UpdateStatus::Type type, const std::string &text)
: m_importance(UpdateStatus::High), m_statusType(type)
{
    COPY_STRING(m_text, text.substr(0, sizeof(m_text) - 1));
}

const char *UpdateStatus::text() const
{
    return m_text.data();
}

UpdateStatus::Importance UpdateStatus::importance() const
{
    return m_importance;
}

UpdateStatus::Type UpdateStatus::statusType() const
{
    return m_statusType;
}

ObjectReceivePolicy::ObjectReceivePolicy(ObjectReceivePolicy::Policy pol): m_policy(pol)
{}

ObjectReceivePolicy::Policy ObjectReceivePolicy::policy() const
{
    return m_policy;
}

SchedulingPolicy::SchedulingPolicy(SchedulingPolicy::Schedule pol): m_policy(pol)
{}

SchedulingPolicy::Schedule SchedulingPolicy::policy() const
{
    return m_policy;
}

ReducePolicy::ReducePolicy(ReducePolicy::Reduce red): m_reduce(red)
{}

ReducePolicy::Reduce ReducePolicy::policy() const
{
    return m_reduce;
}

ExecutionProgress::ExecutionProgress(Progress stage, int count): m_stage(stage), m_executionCount(count)
{}

ExecutionProgress::Progress ExecutionProgress::stage() const
{
    return m_stage;
}

void ExecutionProgress::setStage(ExecutionProgress::Progress stage)
{
    m_stage = stage;
}

void ExecutionProgress::setExecutionCount(int count)
{
    m_executionCount = count;
}

int ExecutionProgress::getExecutionCount() const
{
    return m_executionCount;
}

Trace::Trace(int module, Type messageType, bool onoff): m_module(module), m_messageType(messageType), m_on(onoff)
{}

int Trace::module() const
{
    return m_module;
}

Type Trace::messageType() const
{
    return m_messageType;
}

bool Trace::on() const
{
    return m_on;
}

template<Type MessageType>
ModuleBaseMessage<MessageType>::ModuleBaseMessage(const AvailableModuleBase &mod): m_hub(mod.hub())
{
    COPY_STRING(m_name, mod.name());
    COPY_STRING(m_path, mod.path());
}

template<Type MessageType>
int ModuleBaseMessage<MessageType>::hub() const
{
    return m_hub;
}

template<Type MessageType>
const char *ModuleBaseMessage<MessageType>::name() const
{
    return m_name.data();
}

template<Type MessageType>
const char *ModuleBaseMessage<MessageType>::path() const
{
    return m_path.data();
}

template class ModuleBaseMessage<MODULEAVAILABLE>;
template class ModuleBaseMessage<CREATEMODULECOMPOUND>;

LockUi::LockUi(bool locked): m_locked(locked)
{}

bool LockUi::locked() const
{
    return m_locked;
}

RequestTunnel::RequestTunnel(unsigned short srcPort, const std::string &destHost, unsigned short destPort)
: m_srcPort(srcPort), m_destType(RequestTunnel::Hostname), m_destPort(destPort), m_remove(false)
{
    COPY_STRING(m_destAddr, destHost);
}

RequestTunnel::RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v6 destAddr,
                             unsigned short destPort)
: m_srcPort(srcPort), m_destType(RequestTunnel::IPv6), m_destPort(destPort), m_remove(false)
{
    const std::string addr = destAddr.to_string();
    COPY_STRING(m_destAddr, addr);
}

RequestTunnel::RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v4 destAddr,
                             unsigned short destPort)
: m_srcPort(srcPort), m_destType(RequestTunnel::IPv4), m_destPort(destPort), m_remove(false)
{
    const std::string addr = destAddr.to_string();
    COPY_STRING(m_destAddr, addr);
}

RequestTunnel::RequestTunnel(unsigned short srcPort, unsigned short destPort)
: m_srcPort(srcPort), m_destType(RequestTunnel::Unspecified), m_destPort(destPort), m_remove(false)
{
    const std::string empty;
    COPY_STRING(m_destAddr, empty);
}

RequestTunnel::RequestTunnel(unsigned short srcPort)
: m_srcPort(srcPort), m_destType(RequestTunnel::Unspecified), m_destPort(0), m_remove(true)
{
    const std::string empty;
    COPY_STRING(m_destAddr, empty);
}

unsigned short RequestTunnel::srcPort() const
{
    return m_srcPort;
}

unsigned short RequestTunnel::destPort() const
{
    return m_destPort;
}

RequestTunnel::AddressType RequestTunnel::destType() const
{
    return m_destType;
}

bool RequestTunnel::destIsAddress() const
{
    return m_destType == IPv6 || m_destType == IPv4;
}

void RequestTunnel::setDestAddr(boost::asio::ip::address_v4 addr)
{
    const std::string addrString = addr.to_string();
    COPY_STRING(m_destAddr, addrString);
    m_destType = IPv4;
}

void RequestTunnel::setDestAddr(boost::asio::ip::address_v6 addr)
{
    const std::string addrString = addr.to_string();
    COPY_STRING(m_destAddr, addrString);
    m_destType = IPv6;
}

boost::asio::ip::address RequestTunnel::destAddr() const
{
    assert(destIsAddress());
    if (destType() == IPv6)
        return destAddrV6();
    else
        return destAddrV4();
}

boost::asio::ip::address_v6 RequestTunnel::destAddrV6() const
{
    assert(m_destType == IPv6);
    return boost::asio::ip::address_v6::from_string(m_destAddr.data());
}

boost::asio::ip::address_v4 RequestTunnel::destAddrV4() const
{
    assert(m_destType == IPv4);
    return boost::asio::ip::address_v4::from_string(m_destAddr.data());
}

std::string RequestTunnel::destHost() const
{
    return m_destAddr.data();
}

bool RequestTunnel::remove() const
{
    return m_remove;
}

RequestObject::RequestObject(const AddObject &add, const std::string &objId, const std::string &referrer)
: m_objectId(objId), m_referrer(referrer.empty() ? add.objectName() : referrer), m_array(false), m_arrayType(-1)
{
    setUuid(add.uuid());
    setDestId(add.senderId());
    setDestRank(add.rank());
}

RequestObject::RequestObject(int destId, int destRank, const std::string &objId, const std::string &referrer)
: m_objectId(objId), m_referrer(referrer), m_array(false), m_arrayType(-1)
{
    setDestId(destId);
    setDestRank(destRank);
}

RequestObject::RequestObject(int destId, int destRank, const std::string &arrayId, int type,
                             const std::string &referrer)
: m_objectId(arrayId), m_referrer(referrer), m_array(true), m_arrayType(type)
{
    setDestId(destId);
    setDestRank(destRank);
}

const char *RequestObject::objectId() const
{
    return m_objectId;
}

const char *RequestObject::referrer() const
{
    return m_referrer;
}

bool RequestObject::isArray() const
{
    return m_array;
}

int RequestObject::arrayType() const
{
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
: m_array(true), m_objectId(request.objectId()), m_referrer(request.referrer()), m_objectType(request.arrayType())
{
    setUuid(request.uuid());
    m_payloadSize = payloadSize;
}

const char *SendObject::objectId() const
{
    return m_objectId;
}

const char *SendObject::referrer() const
{
    return m_referrer;
}

const Meta &SendObject::meta() const
{
    return m_meta;
}

Object::Type SendObject::objectType() const
{
    return static_cast<Object::Type>(m_objectType);
}


Meta SendObject::objectMeta() const
{
    Meta meta(m_block, m_timestep, m_animationstep, m_iteration, m_executionCount, m_creator);
    meta.setNumBlocks(m_numBlocks);
    meta.setNumTimesteps(m_numTimesteps);
    meta.setNumAnimationSteps(m_numAnimationsteps);
    meta.setRealTime(m_realtime);
    return meta;
}

bool SendObject::isArray() const
{
    return m_array;
}

FileQuery::FileQuery(int moduleId, const std::string &path, Command command, size_t payloadsize)
: m_command(command), m_moduleId(moduleId)
{
    m_payloadSize = payloadsize;
    COPY_STRING(m_path, path);
}

const char *FileQuery::path() const
{
    return m_path.data();
}

void FileQuery::setFilebrowserId(int id)
{
    m_filebrowser = id;
}

int FileQuery::filebrowserId() const
{
    return m_filebrowser;
}

FileQuery::Command FileQuery::command() const
{
    return Command(m_command);
}

int FileQuery::moduleId() const
{
    return m_moduleId;
}

FileQueryResult::FileQueryResult(const FileQuery &request, Status status, size_t payloadsize)
: m_command(request.command()), m_status(status), m_filebrowser(request.filebrowserId())
{
    setDestId(request.senderId());
    setDestUiId(request.uiId());
    m_payloadSize = payloadsize;
    setReferrer(request.uuid());
    strcpy(m_path.data(), request.m_path.data());
}

const char *FileQueryResult::path() const
{
    return m_path.data();
}

FileQuery::Command FileQueryResult::command() const
{
    return FileQuery::Command(m_command);
}

FileQueryResult::Status FileQueryResult::status() const
{
    return Status(m_status);
}

int FileQueryResult::filebrowserId() const
{
    return m_filebrowser;
}

DataTransferState::DataTransferState(size_t numTransferring): m_numTransferring(numTransferring)
{}

size_t DataTransferState::numTransferring() const
{
    return m_numTransferring;
}

std::ostream &operator<<(std::ostream &s, const Message &m)
{
    using namespace vistle::message;

    s << "uuid: " << boost::lexical_cast<std::string>(m.uuid()) << ", type: " << m.type() << ", size: " << m.size()
      << "+" << m.payloadSize() << ", sender: " << m.senderId();
    if (Id::isHub(m.senderId())) {
        s << ", UI: " << m.uiId();
    } else {
        s << ", rank: " << m.rank();
    }
    s << ", dest: " << m.destId() << ", bcast: " << m.isForBroadcast() << "/" << m.wasBroadcast();
    if (m.destRank() != -1)
        s << ", dest rank: " << m.destRank();

    if (!m.referrer().is_nil())
        s << ", ref: " << boost::lexical_cast<std::string>(m.referrer());

    switch (m.type()) {
    case TRACE: {
        auto &mm = static_cast<const Trace &>(m);
        s << ", module: " << mm.module() << ", type: " << toString(mm.messageType());
        break;
    }
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
        s << ", stage: " << ExecutionProgress::toString(mm.stage()) << ", execcount: " << mm.getExecutionCount();
        break;
    }
    case ADDPARAMETER: {
        auto &mm = static_cast<const AddParameter &>(m);
        s << ", name: " << mm.getName();
        s << ", type: " << Parameter::toString(mm.getParameterType());
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
        s << ", type: " << Parameter::toString(mm.getParameterType());
        if (mm.isInitialization())
            s << " (INIT)";
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
        s << ", from: " << mm.getModuleA() << ":" << mm.getPortAName() << ", to: " << mm.getModuleB() << ":"
          << mm.getPortBName();
        break;
    }
    case DISCONNECT: {
        auto &mm = static_cast<const Disconnect &>(m);
        s << ", from: " << mm.getModuleA() << ":" << mm.getPortAName() << ", to: " << mm.getModuleB() << ":"
          << mm.getPortBName();
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
    case KILL: {
        auto &mm = static_cast<const Kill &>(m);
        s << ", id: " << mm.getModule();
        break;
    }
    case DEBUG: {
        auto &mm = static_cast<const Debug &>(m);
        s << ", req: " << mm.getRequest();
        s << ", id: " << mm.getModule();
        break;
    }
    case ADDHUB: {
        auto &mm = static_cast<const AddHub &>(m);
        s << ", name: " << mm.name() << ", id: " << mm.id() << ", addr: ";
        if (mm.hasAddress()) {
            s << mm.address();
        } else {
            s << "n/a";
        }
        break;
    }
    case SENDTEXT: {
        auto &mm = static_cast<const SendText &>(m);
        s << ", type: " << mm.textType();
        break;
    }
    case ITEMINFO: {
        auto &mm = static_cast<const ItemInfo &>(m);
        s << ", type: " << mm.infoType();
        s << ", port: " << mm.port();
        break;
    }
    case UPDATESTATUS: {
        auto &mm = static_cast<const UpdateStatus &>(m);
        s << ", status: " << mm.text();
        break;
    }
    case ADDOBJECT: {
        auto &mm = static_cast<const AddObject &>(m);
        s << ", obj: " << mm.objectName() << ", " << mm.getSenderPort() << " -> " << mm.getDestPort()
          << " (handle: " << (mm.handleValid() ? "valid" : "invalid") << ")";
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
        s << ", " << (mm.isArray() ? "array" : "object") << ": " << mm.objectId() << ", ref: " << mm.referrer()
          << ", payload size: " << mm.payloadSize();
        break;
    }
    case FILEQUERY: {
        auto &mm = static_cast<const FileQuery &>(m);
        s << ", command: " << mm.command() << ", path: " << mm.path() << ", filebrowser: " << mm.filebrowserId();
        break;
    }
    case FILEQUERYRESULT: {
        auto &mm = static_cast<const FileQueryResult &>(m);
        s << ", status: " << mm.status() << ", path: " << mm.path() << ", filebrowser: " << mm.filebrowserId();
        break;
    }
    case COVER: {
        auto &mm = static_cast<const Cover &>(m);
        s << ", mirror: " << mm.mirrorId() << ", sender: " << mm.sender() << ", sender type: " << mm.senderType()
          << ", subtype: " << mm.subType();
        break;
    }
    default:
        break;
    }

    return s;
}

SetParameterChoices::Payload::Payload() = default;

SetParameterChoices::Payload::Payload(const std::vector<std::string> &choices): choices(choices)
{}

Cover::Cover(int subType)
: m_mirrorId(message::Id::Invalid)
, m_senderId(message::Id::Invalid)
, m_senderType(message::Id::Invalid)
, m_subType(subType)
{}

Cover::Cover(int mirror, int senderId, int senderType, int subType)
: m_mirrorId(mirror), m_senderId(senderId), m_senderType(senderType), m_subType(subType)
{}

int Cover::mirrorId() const
{
    return m_mirrorId;
}

int Cover::sender() const
{
    return m_senderId;
}

int Cover::senderType() const
{
    return m_senderType;
}

int Cover::subType() const
{
    return m_subType;
}


} // namespace message
} // namespace vistle
