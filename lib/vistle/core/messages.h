#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <array>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/address_v4.hpp>

#include <vistle/util/enum.h>
#include "archives_config.h"
#include "export.h"
#include "message.h"
#include "objectmeta.h"
#include "object.h"
#include "parameter.h"
#include "paramvector.h"
#include "scalar.h"
#include "shmname.h"
#include "uuid.h"

#pragma pack(push)
#pragma pack(1)

namespace vistle {

class Communicator;
class Parameter;
class Port;
class AvailableModuleBase;
namespace message {

//! indicate the kind of a communication partner
class V_COREEXPORT Identify: public MessageBase<Identify, IDENTIFY> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Identity,
                                        (UNKNOWN)(REQUEST) //< request receiver to send its identity
                                        (UI) //< user interface
                                        (MANAGER) //< cluster manager
                                        (HUB) //< master hub
                                        (SLAVEHUB) //< slave hub
                                        (LOCALBULKDATA) //< bulk data transfer to local MPI ranks
                                        (REMOTEBULKDATA) //< bulk data transfer to remote hubs
                                        (RENDERSERVER) //< remote render server
                                        (RENDERCLIENT) //< remote render client
                                        (VRB) //< COVISE/OpenCOVER request broker for collaborative VR
    )

    typedef std::array<char, 32> mac_t;
    typedef std::array<char, 64> session_data_t;

    explicit Identify(const std::string &name = ""); //< request identity
    Identify(const Identify &request, Identity id, const std::string &name = ""); //< answer identification request
    Identify(const Identify &request, Identity id, int rank);
    Identity identity() const;
    const char *name() const;
    int rank() const;
    int numRanks() const;
    int boost_archive_version() const;
    int indexSize() const;
    int scalarSize() const;

    void setNumRanks(int size);
    unsigned long pid() const;
    void setPid(unsigned long pid);

    void computeMac();
    bool verifyMac(bool compareSessionData = true) const;

private:
    Identity m_identity;
    description_t m_name;
    int m_numRanks;
    int m_rank;
    int m_boost_archive_version;
    int m_indexSize, m_scalarSize;
    unsigned long m_pid = 0;
    session_data_t m_session_data;
    mac_t m_mac;
};
V_ENUM_OUTPUT_OP(Identity, Identify)

//! announce that a (slave) hub has connected
class V_COREEXPORT AddHub: public MessageBase<AddHub, ADDHUB> {
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(AddressType, (Hostname)(IPv4)(IPv6)(Unspecified))

public:
    AddHub(int id, const std::string &name);
    int id() const;
    const char *name() const;
    const char *loginName() const;
    const char *realName() const;
    int numRanks() const;
    unsigned short port() const;
    unsigned short dataPort() const;
    AddressType addressType() const;
    bool hasAddress() const;
    std::string host() const;
    boost::asio::ip::address address() const;
    boost::asio::ip::address_v6 addressV6() const;
    boost::asio::ip::address_v4 addressV4() const;

    void setNumRanks(int size);
    void setPort(unsigned short port);
    void setDataPort(unsigned short port);
    void setAddress(boost::asio::ip::address addr);
    void setAddress(boost::asio::ip::address_v6 addr);
    void setAddress(boost::asio::ip::address_v4 addr);

    void setLoginName(const std::string &login);
    void setRealName(const std::string &real);

    bool hasUserInterface() const;
    void setHasUserInterface(bool ui);

    bool hasVrb() const;
    void setHasVrb(bool vrb);

    std::string systemType() const;
    void setSystemType(const std::string &os);
    std::string arch() const;
    void setArch(const std::string &arch);

private:
    int m_id;
    address_t m_name;
    address_t m_loginName;
    address_t m_realName;

    int m_numRanks;
    unsigned short m_port;
    unsigned short m_dataPort;
    AddressType m_addrType;
    address_t m_address;
    bool m_hasUserInterface;
    bool m_hasVrb;
    port_name_t m_systemType;
    port_name_t m_arch;
};

//! request that a slave hub be deleted
class V_COREEXPORT RemoveHub: public MessageBase<RemoveHub, REMOVEHUB> {
public:
    explicit RemoveHub(int id);
    int id() const;

private:
    int m_id;
};

//! debug: request a reply containing character 'c'
class V_COREEXPORT Ping: public MessageBase<Ping, PING> {
public:
    explicit Ping(const char c);

    char getCharacter() const;

private:
    const char character;
};

//! debug: reply to pong
class V_COREEXPORT Pong: public MessageBase<Pong, PONG> {
public:
    explicit Pong(const Ping &ping);

    char getCharacter() const;
    int getDestination() const;

private:
    const char character;
    int module;
};

//! spawn a module
class V_COREEXPORT Spawn: public MessageBase<Spawn, SPAWN> {
public:
    Spawn(int hubId, const std::string &name, int size = -1, int baserank = -1, int rankskip = -1);

    void setMigrateId(int id);
    int migrateId() const;
    void setMirroringId(int id);
    int mirroringId() const;
    int hubId() const;
    void setHubId(int id);
    int spawnId() const;
    void setSpawnId(int id);
    void setName(const char *modname);
    const char *getName() const;
    int getMpiSize() const;
    int getBaseRank() const;
    int getRankSkip() const;

private:
    //! id of hub where to spawn module
    int m_hub;
    //! ID of module to spawn
    int m_spawnId;
    //! number of ranks in communicator
    int mpiSize;
    //! first rank on which to spawn process
    int baseRank;
    //! number of ranks to skip when spawning process
    int rankSkip;
    //! id of module to migrate
    int m_migrateId = Id::Invalid;
    //! id of module that is being mirrored (for replicating COVER renderer on clusters with UI)
    int m_mirroringId = Id::Invalid;
    //! name of module to be started
    module_name_t name;
};

//! notification of manager that spawning is possible (i.e. shmem has been set up)
class V_COREEXPORT SpawnPrepared: public MessageBase<SpawnPrepared, SPAWNPREPARED> {
public:
    explicit SpawnPrepared(const Spawn &spawn);

    int hubId() const;
    int spawnId() const;
    void setSpawnId(int id);
    const char *getName() const;

private:
    //! id of hub where to spawn module
    int m_hub;
    //! ID of module to spawn
    int m_spawnId;
    //! name of module to be started
    module_name_t name;
};

//! acknowledge that a module has been spawned
class V_COREEXPORT Started: public MessageBase<Started, STARTED> {
public:
    explicit Started(const std::string &name);

    const char *getName() const;
    void setPid(unsigned long pid);
    unsigned long pid() const;

private:
    //! name of module to be started
    module_name_t name;
    unsigned long m_pid = 0;
};

//! request a module to quit
class V_COREEXPORT Kill: public MessageBase<Kill, KILL> {
public:
    explicit Kill(const int module);

    int getModule() const;

private:
    //! ID of module to stop
    const int module;
};

//! request all modules to quit for terminating the session
class V_COREEXPORT Quit: public MessageBase<Quit, QUIT> {
public:
    explicit Quit(int id = Id::Broadcast);
    int id() const;

private:
    const int m_id;
};

//! request a module to quit
class V_COREEXPORT Debug: public MessageBase<Debug, DEBUG> {
public:
    explicit Debug(const int module);

    int getModule() const;

private:
    //! ID of module to stop
    const int module;
};

//! notify that a module has quit
class V_COREEXPORT ModuleExit: public MessageBase<ModuleExit, MODULEEXIT> {
public:
    ModuleExit();
    void setForwarded();
    bool isForwarded() const;

private:
    bool forwarded;
};

//! instruct GUI to store a snapshot of the rendered workflow
class V_COREEXPORT Screenshot: public MessageBase<Screenshot, SCREENSHOT> {
public:
    explicit Screenshot(const std::string &filename);
    const char *filename() const;

private:
    path_t m_filename;
};
class V_COREEXPORT Execute: public MessageBase<Execute, EXECUTE> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(What,
                                        (Prepare) // call prepare()
                                        (ComputeExecute) // call compute() - because this module was executed
                                        (ComputeObject) // call compute() - because objects have been received
                                        (Reduce) // call reduce()
    )

    explicit Execute(What what = Execute::ComputeExecute, int module = Id::Broadcast, int count = -1);
    Execute(int module, double realtime, double step = 0.);

    void setModule(int);
    int getModule() const;
    void setExecutionCount(int count);
    int getExecutionCount() const;

    bool allRanks() const;
    void setAllRanks(bool allRanks);

    What what() const;
    void setWhat(What r);

    double animationRealTime() const;
    double animationStepDuration() const;

private:
    bool m_allRanks; //!< whether execute should be broadcasted across all MPI ranks
    int module; //!< destination module, -1: all sources
    int executionCount; //!< count of execution which triggered this execute
    What m_what; //!< reason why this message was generated
    double m_realtime; //!< realtime/timestep currently displayed
    double m_animationStepDuration; //!< duration of a single timestep
};
V_ENUM_OUTPUT_OP(What, Execute)

//! notify that module is done executing
class V_COREEXPORT ExecutionDone: public MessageBase<ExecutionDone, EXECUTIONDONE> {
public:
    explicit ExecutionDone(int executionCount);
    int getExecutionCount() const;

private:
    int m_executionCount; //!< count of execution after which this message was issued
};

//! trigger execution of a module function
class V_COREEXPORT CancelExecute: public MessageBase<CancelExecute, CANCELEXECUTE> {
public:
    explicit CancelExecute(const int module);
    int getModule() const;

private:
    int m_module;
};

//! indicate that a module has started computing
class V_COREEXPORT Busy: public MessageBase<Busy, BUSY> {
public:
    Busy();

private:
};

//! indicate that a module has finished computing
class V_COREEXPORT Idle: public MessageBase<Idle, IDLE> {
public:
    Idle();

private:
};

//! notification that a module has created an input/output port
class V_COREEXPORT AddPort: public MessageBase<AddPort, ADDPORT> {
public:
    explicit AddPort(const Port &port);
    Port getPort() const;

private:
    port_name_t m_name;
    description_t m_description;
    int m_porttype;
    int m_flags;
};

//! notification that a module has destroyed an input/output port
class V_COREEXPORT RemovePort: public MessageBase<RemovePort, REMOVEPORT> {
public:
    explicit RemovePort(const Port &port);
    Port getPort() const;

private:
    port_name_t m_name;
};

class AddObjectCompleted;

//! add an object to the input queue of an input port
class V_COREEXPORT AddObject: public MessageBase<AddObject, ADDOBJECT> {
public:
    AddObject(const std::string &senderPort, vistle::Object::const_ptr obj, const std::string &destPort = "");
    explicit AddObject(const AddObject &other);
    explicit AddObject(const AddObjectCompleted &complete);
    ~AddObject();

    bool operator<(const AddObject &other) const;

    void setSenderPort(const std::string &sendPort);
    const char *getSenderPort() const;
    void setDestPort(const std::string &destPort);
    const char *getDestPort() const;
    const char *objectName() const;
    void setObject(Object::const_ptr obj);
    Object::const_ptr takeObject() const; //!< may only be called once
    Object::const_ptr getObject() const; //! try to retrieve from shmem by name
    bool ref() const; //!< may only be called once
    const Meta &meta() const;
    Object::Type objectType() const;
    const shm_handle_t &getHandle() const;
    bool handleValid() const;

    void setBlocker();
    bool isBlocker() const;
    void setUnblocking();
    bool isUnblocking() const;

private:
    Meta m_meta;
    int m_objectType = Object::UNKNOWN;
    port_name_t senderPort;
    port_name_t destPort;
    shm_name_t m_name;
    shmsegname_t m_shmname;
    shm_handle_t handle = 0;
    mutable bool m_handleValid = false;
    bool m_blocker = false;
    bool m_unblock = false;
};

class V_COREEXPORT AddObjectCompleted: public MessageBase<AddObjectCompleted, ADDOBJECTCOMPLETED> {
public:
    explicit AddObjectCompleted(const AddObject &msg);
    const char *objectName() const;
    int originalDestination() const;
    const char *originalDestPort() const;

private:
    shm_name_t m_name;
    int m_orgDestId;
    port_name_t m_orgDestPort;
};

//! Base class for connect and disconnect
template<Type MessageType>
class V_COREEXPORT ConnectBase: public MessageBase<ConnectBase<MessageType>, MessageType> {
public:
    ConnectBase(const int moduleIDA, const std::string &portA, const int moduleIDB, const std::string &portB);

    const char *getPortAName() const;
    const char *getPortBName() const;

    void setModuleA(int id);
    int getModuleA() const;
    void setModuleB(int id);
    int getModuleB() const;

    void reverse(); //! swap source and destination

private:
    port_name_t portAName;
    port_name_t portBName;

    int moduleA;
    int moduleB;
    static_assert(MessageType == CONNECT || MessageType == DISCONNECT, "wrong Type for ConnectionData");
};

//! connect an output port to an input port of another module
extern template class V_COREEXPORT ConnectBase<CONNECT>;
typedef ConnectBase<CONNECT> Connect;
static_assert(sizeof(Connect) <= Message::MESSAGE_SIZE, "message too large");
//! disconnect an output port to an input port of another module
extern template class V_COREEXPORT ConnectBase<DISCONNECT>;
typedef ConnectBase<DISCONNECT> Disconnect;
static_assert(sizeof(Disconnect) <= Message::MESSAGE_SIZE, "message too large");

//! notification that a module has created a parameter
class V_COREEXPORT AddParameter: public MessageBase<AddParameter, ADDPARAMETER> {
public:
    AddParameter(const Parameter &param, const std::string &moduleName);

    const char *getName() const;
    const char *moduleName() const;
    const char *description() const;
    const char *group() const;
    bool isGroupExpanded() const;
    int getParameterType() const;
    int getPresentation() const;
    std::shared_ptr<Parameter>
    getParameter() const; //< allocates a new Parameter object, caller is responsible for deletion

private:
    param_name_t name;
    param_name_t m_group;
    module_name_t module;
    description_t m_description;
    int paramtype;
    int presentation;
    bool m_groupExpanded;
};

//! notification that a module has removed a parameter
class V_COREEXPORT RemoveParameter: public MessageBase<RemoveParameter, REMOVEPARAMETER> {
public:
    RemoveParameter(const Parameter &param, const std::string &moduleName);

    const char *getName() const;
    const char *moduleName() const;
    int getParameterType() const;
    std::shared_ptr<Parameter>
    getParameter() const; //< allocates a new Parameter object, caller is responsible for deletion

private:
    param_name_t name;
    module_name_t module;
    int paramtype;
};

//! request parameter value update or notify that a parameter value has been changed
class V_COREEXPORT SetParameter: public MessageBase<SetParameter, SETPARAMETER> {
public:
    explicit SetParameter(int module); //<! apply delayed parameter changes
    SetParameter(int module, const std::string &name, const std::shared_ptr<Parameter> param,
                 Parameter::RangeType rt = Parameter::Value, bool defaultValue = false);
    SetParameter(int module, const std::string &name, const Integer value);
    SetParameter(int module, const std::string &name, const Float value);
    SetParameter(int module, const std::string &name, const ParamVector &value);
    SetParameter(int module, const std::string &name, const IntParamVector &value);
    SetParameter(int module, const std::string &name, const std::string &value);

    void setInit();
    bool isInitialization() const;
    void setDelayed();
    bool isDelayed() const;
    void setModule(int);
    int getModule() const;
    bool setType(int type);

    void setRangeType(int rt);
    int rangeType() const;

    const char *getName() const;
    int getParameterType() const;

    Integer getInteger() const;
    std::string getString() const;
    Float getFloat() const;
    ParamVector getVector() const;
    IntParamVector getIntVector() const;

    void setReadOnly(bool readOnly);
    bool isReadOnly() const;

    void setImmediate(bool immed);
    bool isImmediate() const;

    bool apply(std::shared_ptr<Parameter> param) const;

private:
    int m_module; //!< destination module
    int paramtype; //!< parameter type
    int dim; //!< dimensionality for vector parameters
    int rangetype; //!< set parameter bounds instead of parameter value
    union {
        Integer v_int;
        Float v_scalar;
        Float v_vector[MaxDimension];
        Integer v_ivector[MaxDimension];
        param_value_t v_string;
    };
    param_name_t name; //!< parameter name
    bool initialize; //!< called for setting parameter default value
    bool reply; //!< this messaege is in reply to a request to change a parameter and contains the value actually used
    bool delayed; //!< true: wait until parameterChanged should be called
    bool read_only_valid = false; //!< whether read_only was set
    bool read_only = false; //!< true: parameter can only be changed by module
    bool immediate_valid = false; //!< whether immediate was set
    bool immediate = false; //!< true: changes are communicated with higher priority
};

//! set list of choice descriptions for a choice parameter
class V_COREEXPORT SetParameterChoices: public MessageBase<SetParameterChoices, SETPARAMETERCHOICES> {
public:
    struct Payload {
        Payload();
        Payload(const std::vector<std::string> &choices);

        std::vector<std::string> choices;

        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &choices;
        }
    };

    SetParameterChoices(const std::string &name, unsigned numChoices);

    const char *getName() const;
    int getNumChoices() const;

    bool apply(std::shared_ptr<Parameter> param, const Payload &payload) const;

private:
    int numChoices;
    param_name_t name;
};

class V_COREEXPORT Barrier: public MessageBase<Barrier, BARRIER> {
public:
    Barrier();
};

class V_COREEXPORT BarrierReached: public MessageBase<BarrierReached, BARRIERREACHED> {
public:
    explicit BarrierReached(const uuid_t &uuid);
};

class V_COREEXPORT SetId: public MessageBase<SetId, SETID> {
public:
    explicit SetId(const int id);

    int getId() const;

private:
    const int m_id;
};

class V_COREEXPORT ReplayFinished: public MessageBase<ReplayFinished, REPLAYFINISHED> {
public:
    ReplayFinished();
};

//! send text messages to user interfaces
class V_COREEXPORT SendText: public MessageBase<SendText, SENDTEXT> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(TextType, (Cout)(Cerr)(Clog)(Info)(Warning)(Error))

    struct V_COREEXPORT Payload {
        Payload();
        Payload(const std::string &text);

        std::string text;

        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &text;
        }
    };

    //! Error message in response to a Message
    explicit SendText(const Message &inResponseTo);
    explicit SendText(TextType type);

    TextType textType() const;
    Type referenceType() const;
    uuid_t referenceUuid() const;

private:
    //! type of text
    TextType m_textType;
    //! uuid of Message this message is a response to
    uuid_t m_referenceUuid;
    //! Type of Message this message is a response to
    Type m_referenceType;
};
V_ENUM_OUTPUT_OP(TextType, SendText)

//! update status of a module (or other entity)
class V_COREEXPORT UpdateStatus: public MessageBase<UpdateStatus, UPDATESTATUS> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Importance, (Bulk)(Low)(Medium)(High))
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Type, (Text)(LoadedFile)(SessionUrl))

    struct Payload {
        Payload();
        Payload(const std::string &status);

        std::string status;

        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &status;
        }
    };

    explicit UpdateStatus(const std::string &text, Importance prio = Low);
    UpdateStatus(Type type, const std::string &text);
    const char *text() const;

    Importance importance() const;
    Type statusType() const;

private:
    //! message text
    description_t m_text;
    //! message importance
    Importance m_importance;
    //! status type
    Type m_statusType;
};
V_ENUM_OUTPUT_OP(Importance, UpdateStatus)


//! control where AddObject messages are sent
class V_COREEXPORT ObjectReceivePolicy: public MessageBase<ObjectReceivePolicy, OBJECTRECEIVEPOLICY> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Policy, (Local)(Master)(Distribute))
    explicit ObjectReceivePolicy(Policy pol);
    Policy policy() const;

private:
    Policy m_policy;
};
V_ENUM_OUTPUT_OP(Policy, ObjectReceivePolicy)

class V_COREEXPORT SchedulingPolicy: public MessageBase<SchedulingPolicy, SCHEDULINGPOLICY> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(
        Schedule,
        (Ignore) //< prepare/compute/reduce not called at all (e.g. for renderers)
        (Single) //< compute called on each rank individually once per received object
        (Gang) //< compute called on all ranks together once per received object
        (LazyGang) //< compute called on all ranks together, but not necessarily for each received object
    )
    explicit SchedulingPolicy(Schedule pol);
    Schedule policy() const;

private:
    Schedule m_policy;
};
V_ENUM_OUTPUT_OP(Schedule, SchedulingPolicy)

//! control whether/when prepare() and reduce() are called
class V_COREEXPORT ReducePolicy: public MessageBase<ReducePolicy, REDUCEPOLICY> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(
        Reduce,
        (Never) //< module's prepare()/reduce() methods will never be called - only for modules with COMBINE port (renderers)
        (Locally) //< module's prepare()/reduce() methods will be called once unsynchronized on each rank
        (PerTimestep) //< module's reduce() method will be called on all ranks together once per timestep
        (PerTimestepOrdered) //< module's reduce() method will be called on all ranks together once per timestep in ascending order
        (PerTimestepZeroFirst) //< module's reduce() method will be called on all ranks together once per timestep in arbitrary order, but zero first
        (OverAll) //< module's prepare()/reduce() method will be called on all ranks together after all timesteps have been received
    )
    explicit ReducePolicy(Reduce red);
    Reduce policy() const;

private:
    Reduce m_reduce;
};
V_ENUM_OUTPUT_OP(Reduce, ReducePolicy)

//! steer execution stages
/*!
 * module ranks notify the cluster manager about their execution stage
 *
 *
 */
class V_COREEXPORT ExecutionProgress: public MessageBase<ExecutionProgress, EXECUTIONPROGRESS> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Progress,
                                        (Start) //< execution starts - if applicable, prepare() will be invoked
                                        (Finish) //< execution finishes - if applicable, reduce() has finished
    )
    ExecutionProgress(Progress stage, int count);
    Progress stage() const;
    void setStage(Progress stage);
    void setExecutionCount(int count);
    int getExecutionCount() const;

private:
    Progress m_stage;
    int m_executionCount; //!< count of execution which triggered this message
};
V_ENUM_OUTPUT_OP(Progress, ExecutionProgress)

//! enable/disable message tracing for a module
class V_COREEXPORT Trace: public MessageBase<Trace, TRACE> {
public:
    Trace(int module, Type type, bool onoff);
    int module() const;
    Type messageType() const;
    bool on() const;

private:
    int m_module;
    Type m_messageType;
    bool m_on;
};

template<Type MessageType>
class V_COREEXPORT ModuleBaseMessage: public MessageBase<ModuleBaseMessage<MessageType>, MessageType> {
public:
    explicit ModuleBaseMessage(const AvailableModuleBase &mod);
    const char *name() const;
    const char *path() const;
    int hub() const;

private:
    int m_hub;
    module_name_t m_name;
    path_t m_path;
};

//! announce availability of a module to UI
extern template class V_COREEXPORT ModuleBaseMessage<MODULEAVAILABLE>;
typedef ModuleBaseMessage<MODULEAVAILABLE> ModuleAvailable;
static_assert(sizeof(ModuleAvailable) <= Message::MESSAGE_SIZE, "message too large");
//! tell Vistle to create and save a module compound
extern template class V_COREEXPORT ModuleBaseMessage<CREATEMODULECOMPOUND>;
typedef ModuleBaseMessage<CREATEMODULECOMPOUND> CreateModuleCompound;
static_assert(sizeof(CreateModuleCompound) <= Message::MESSAGE_SIZE, "message too large");

//! lock UI (block user interaction)
class V_COREEXPORT LockUi: public MessageBase<LockUi, LOCKUI> {
public:
    explicit LockUi(bool locked);
    bool locked() const;

private:
    bool m_locked;
};

//! request hub to listen on TCP port and forward incoming connections
class V_COREEXPORT RequestTunnel: public MessageBase<RequestTunnel, REQUESTTUNNEL> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(AddressType, (Hostname)(IPv4)(IPv6)(Unspecified))
    //! establish tunnel - let hub forward incoming connections to srcPort to destHost::destPort
    RequestTunnel(unsigned short srcPort, const std::string &destHost, unsigned short destPort);
    //! establish tunnel - let hub forward incoming connections to srcPort to destHost::destPort
    RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v4 destHost, unsigned short destPort);
    //! establish tunnel - let hub forward incoming connections to srcPort to destHost::destPort
    RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v6 destHost, unsigned short destPort);
    //! establish tunnel - let hub forward incoming connections to srcPort to destPort on local interface, address will be filled in by rank 0 of cluster manager
    RequestTunnel(unsigned short srcPort, unsigned short destPort);
    //! remove tunnel
    explicit RequestTunnel(unsigned short srcPort);

    void setDestAddr(boost::asio::ip::address_v6 addr);
    void setDestAddr(boost::asio::ip::address_v4 addr);
    unsigned short srcPort() const;
    unsigned short destPort() const;
    AddressType destType() const;
    bool destIsAddress() const;
    std::string destHost() const;
    boost::asio::ip::address destAddr() const;
    boost::asio::ip::address_v6 destAddrV6() const;
    boost::asio::ip::address_v4 destAddrV4() const;
    bool remove() const;

private:
    unsigned short m_srcPort;
    AddressType m_destType;
    address_t m_destAddr;
    unsigned short m_destPort;
    bool m_remove;
};

//! request remote data object
class V_COREEXPORT RequestObject: public MessageBase<RequestObject, REQUESTOBJECT> {
public:
    RequestObject(const AddObject &add, const std::string &objId, const std::string &referrer = "");
    RequestObject(int destId, int destRank, const std::string &objId, const std::string &referrer);
    RequestObject(int destId, int destRank, const std::string &arrayId, int type, const std::string &referrer);
    const char *objectId() const;
    const char *referrer() const;
    bool isArray() const;
    int arrayType() const;

private:
    shm_name_t m_objectId;
    shm_name_t m_referrer;
    bool m_array;
    int m_arrayType;
};

//! header for data object transmission
class V_COREEXPORT SendObject: public MessageBase<SendObject, SENDOBJECT> {
public:
    SendObject(const RequestObject &request, vistle::Object::const_ptr obj, size_t payloadSize);
    SendObject(const RequestObject &request, size_t payloadSize);
    const char *objectId() const;
    const char *referrer() const;
    const Meta &meta() const;
    Object::Type objectType() const;
    Meta objectMeta() const;
    bool isArray() const;

private:
    bool m_array;
    shm_name_t m_objectId;
    shm_name_t m_referrer;
    int m_objectType;
    Meta m_meta;
    int32_t m_block, m_numBlocks;
    int32_t m_timestep, m_numTimesteps;
    int32_t m_animationstep, m_numAnimationsteps;
    int32_t m_iteration;
    int32_t m_executionCount;
    int32_t m_creator;
    double m_realtime;
};

class V_COREEXPORT FileQuery: public MessageBase<FileQuery, FILEQUERY> {
    friend class FileQueryResult;

public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Command, (SystemInfo)(LookUpFiles)(ReadDirectory)(MakeDirectory))
    FileQuery(int moduleId, const std::string &path, Command cmd, size_t payloadsize = 0);
    Command command() const;
    int moduleId() const;
    const char *path() const;
    void setFilebrowserId(int id);
    int filebrowserId() const;

private:
    int m_command;
    int m_moduleId;
    int m_filebrowser = -1;
    path_t m_path;
};

class V_COREEXPORT FileQueryResult: public MessageBase<FileQuery, FILEQUERYRESULT> {
public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Status, (Ok)(Error)(DoesNotExist)(NoPermission))
    FileQueryResult(const FileQuery &request, Status status, size_t payloadsize);
    const char *path() const;
    FileQuery::Command command() const;
    Status status() const;
    int filebrowserId() const;

private:
    int m_command;
    int m_status;
    int m_filebrowser;
    path_t m_path;
};

class V_COREEXPORT DataTransferState: public MessageBase<DataTransferState, DATATRANSFERSTATE> {
public:
    DataTransferState(size_t numTransferring);
    size_t numTransferring() const;

private:
    long m_numTransferring;
};

//! wrap a COVISE message sent by COVER
class V_COREEXPORT Cover: public MessageBase<Cover, COVER> {
public:
    Cover(int mirror, int senderId, int senderType, int subType);

    int sender() const;
    int senderType() const;
    int subType() const;
    int mirrorId() const;

private:
    int m_mirrorId;
    int m_senderId;
    int m_senderType;
    int m_subType;
};
//wrapper for COVER coGRMSG
class V_COREEXPORT coGRMsg: public MessageBase<coGRMsg, COGRMSG> {
public:
    struct Payload {
        Payload();
        Payload(const std::string &content);

        std::string content;

        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &content;
        }
    };
    coGRMsg();
};

template<class Payload>
extern V_COREEXPORT buffer addPayload(Message &message, const Payload &payload);
template<class Payload>
extern V_COREEXPORT Payload getPayload(const buffer &data);

extern template V_COREEXPORT buffer addPayload<std::string>(Message &message, const std::string &payload);
extern template V_COREEXPORT buffer addPayload<SendText::Payload>(Message &message, const SendText::Payload &payload);
extern template V_COREEXPORT buffer
addPayload<SetParameterChoices::Payload>(Message &message, const SetParameterChoices::Payload &payload);
extern template V_COREEXPORT buffer addPayload<coGRMsg::Payload>(Message &message, const coGRMsg::Payload &payload);

extern template V_COREEXPORT std::string getPayload(const buffer &data);
extern template V_COREEXPORT SendText::Payload getPayload(const buffer &data);
extern template V_COREEXPORT SetParameterChoices::Payload getPayload(const buffer &data);
extern template V_COREEXPORT coGRMsg::Payload getPayload(const buffer &data);

V_COREEXPORT std::ostream &operator<<(std::ostream &s, const Message &msg);

//! terminate a socket connection
class V_COREEXPORT CloseConnection: public MessageBase<CloseConnection, CLOSECONNECTION> {
public:
    CloseConnection(const std::string &reason);
    const char *reason() const;

private:
    description_t m_reason;
};

} // namespace message
} // namespace vistle

#pragma pack(pop)
#endif
