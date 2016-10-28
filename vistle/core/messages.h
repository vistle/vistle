#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>
#include <array>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/address_v4.hpp>

#include <util/enum.h>
#include <util/directory.h>
#include "uuid.h"
#include "object.h"
#include "scalar.h"
#include "paramvector.h"
#include "parameter.h"
#include "message.h"
#include "export.h"

#pragma pack(push)
#pragma pack(1)

namespace vistle {

class Communicator;
class Parameter;
class Port;

namespace message {

template<class MessageClass, Message::Type MessageType>
class MessageBase: public Message {
public:
    static const Message::Type s_type = MessageType;
protected:
    MessageBase(): Message(MessageType, sizeof(MessageClass)) {
    }
};

//! indicate the kind of a communication partner
class V_COREEXPORT Identify: public MessageBase<Identify, Message::IDENTIFY> {

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Identity,
         (UNKNOWN)
         (REQUEST) //< request receiver to send its identity
         (UI) //< user interface
         (MANAGER) //< cluster manager
         (HUB) //< master hub
         (SLAVEHUB) //< slave hub
         (LOCALBULKDATA) //< bulk data transfer to local MPI ranks
         (REMOTEBULKDATA) //< bulk data transfer to remote hubs
         );

   Identify(Identity id, const std::string &name = "");
   Identify(Identity id, int rank);
   Identity identity() const;
   const char *name() const;
   int id() const;
   int rank() const;

 private:
   Identity m_identity;
   text_t m_name;
   int m_id;
   int m_rank;

};
BOOST_STATIC_ASSERT(sizeof(Identify) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(Identity, Identify)

//! announce that a slave hub has connected
class V_COREEXPORT AddHub: public MessageBase<AddHub, Message::ADDHUB> {

   DEFINE_ENUM_WITH_STRING_CONVERSIONS(AddressType,
      (Hostname)
      (IPv4)
      (IPv6)
      (Unspecified)
   )

 public:
   AddHub(int id, const std::string &name);
   int id() const;
   const char *name() const;
   unsigned short port() const;
   unsigned short dataPort() const;
   AddressType addressType() const;
   bool hasAddress() const;
   std::string host() const;
   boost::asio::ip::address address() const;
   boost::asio::ip::address_v6 addressV6() const;
   boost::asio::ip::address_v4 addressV4() const;

   void setPort(unsigned short port);
   void setDataPort(unsigned short port);
   void setAddress(boost::asio::ip::address addr);
   void setAddress(boost::asio::ip::address_v6 addr);
   void setAddress(boost::asio::ip::address_v4 addr);

 private:
   int m_id;
   address_t m_name;
   unsigned short m_port;
   unsigned short m_dataPort;
   AddressType m_addrType;
   address_t m_address;

};
BOOST_STATIC_ASSERT(sizeof(AddHub) <= Message::MESSAGE_SIZE);

//! debug: request a reply containing character 'c'
class V_COREEXPORT Ping: public MessageBase<Ping, Message::PING> {

 public:
   Ping(const char c);

   char getCharacter() const;

 private:
   const char character;
};
BOOST_STATIC_ASSERT(sizeof(Ping) <= Message::MESSAGE_SIZE);

//! debug: reply to pong
class V_COREEXPORT Pong: public MessageBase<Pong, Message::PONG> {

 public:
   Pong(const Ping &ping);

   char getCharacter() const;
   int getDestination() const;

 private:
   const char character;
   int module;
};
BOOST_STATIC_ASSERT(sizeof(Pong) <= Message::MESSAGE_SIZE);

//! spawn a module
class V_COREEXPORT Spawn: public MessageBase<Spawn, Message::SPAWN> {

 public:
   Spawn(int hubId, const std::string &name, int size=-1, int baserank=-1, int rankskip=-1);

   int hubId() const;
   int spawnId() const;
   void setSpawnId(int id);
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
   //! name of module to be started
   module_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(Spawn) <= Message::MESSAGE_SIZE);

//! notification of manager that spawning is possible (i.e. shmem has been set up)
class V_COREEXPORT SpawnPrepared: public MessageBase<SpawnPrepared, Message::SPAWNPREPARED> {

 public:
   SpawnPrepared(const Spawn &spawn);

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
BOOST_STATIC_ASSERT(sizeof(SpawnPrepared) <= Message::MESSAGE_SIZE);

//! acknowledge that a module has been spawned
class V_COREEXPORT Started: public MessageBase<Started, Message::STARTED> {

 public:
   Started(const std::string &name);

   const char *getName() const;

 private:
   //! name of module to be started
   module_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(Started) <= Message::MESSAGE_SIZE);

//! request a module to quit
class V_COREEXPORT Kill: public MessageBase<Kill, Message::KILL> {

 public:
   Kill(const int module);

   int getModule() const;

 private:
   //! ID of module to stop
   const int module;
};
BOOST_STATIC_ASSERT(sizeof(Kill) <= Message::MESSAGE_SIZE);

//! request all modules to quit for terminating the session
class V_COREEXPORT Quit: public MessageBase<Quit, Message::QUIT> {

 public:
   Quit();

 private:
};
BOOST_STATIC_ASSERT(sizeof(Quit) <= Message::MESSAGE_SIZE);

//! notify that a module has quit
class V_COREEXPORT ModuleExit: public MessageBase<ModuleExit, Message::MODULEEXIT> {

 public:
   ModuleExit();
   void setForwarded();
   bool isForwarded() const;

 private:
   bool forwarded;
};
BOOST_STATIC_ASSERT(sizeof(ModuleExit) <= Message::MESSAGE_SIZE);

//! trigger execution of a module function
class V_COREEXPORT Execute: public MessageBase<Execute, Message::EXECUTE> {

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(What,
      (Prepare) // call prepare()
      (ComputeExecute) // call compute() - because this module was executed
      (ComputeObject) // call compute() - because objects have been received
      (Reduce) // call reduce()
   )

   Execute(What what=Execute::ComputeExecute, const int module=Id::Broadcast, const int count=-1);

   void setModule(int );
   int getModule() const;
   void setExecutionCount(int count);
   int getExecutionCount() const;

   bool allRanks() const;
   void setAllRanks(bool allRanks);

   What what() const;
   void setWhat(What r);

private:
   bool m_allRanks; //!< whether execute should be broadcasted across all MPI ranks
   int module; //!< destination module, -1: all sources
   int executionCount; //!< count of execution which triggered this execute
   What m_what; //!< reason why this message was generated
};
BOOST_STATIC_ASSERT(sizeof(Execute) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(What, Execute)

//! indicate that a module has started computing
class V_COREEXPORT Busy: public MessageBase<Busy, Message::BUSY> {

 public:
   Busy();

 private:
};
BOOST_STATIC_ASSERT(sizeof(Busy) <= Message::MESSAGE_SIZE);

//! indicate that a module has finished computing
class V_COREEXPORT Idle: public MessageBase<Idle, Message::IDLE> {

 public:
   Idle();

 private:
};
BOOST_STATIC_ASSERT(sizeof(Idle) <= Message::MESSAGE_SIZE);

//! notification that a module has created an input/output port
class V_COREEXPORT AddPort: public MessageBase<AddPort, Message::ADDPORT> {

 public:
   AddPort(const Port &port);
   Port getPort() const;
 private:
   port_name_t m_name;
   text_t m_description;
   int m_porttype;
   int m_flags;
};
BOOST_STATIC_ASSERT(sizeof(AddPort) <= Message::MESSAGE_SIZE);

//! notification that a module has destroyed an input/output port
class V_COREEXPORT RemovePort: public MessageBase<RemovePort, Message::REMOVEPORT> {

 public:
   RemovePort(const Port &port);
   Port getPort() const;
 private:
   port_name_t m_name;
};
BOOST_STATIC_ASSERT(sizeof(RemovePort) <= Message::MESSAGE_SIZE);

class AddObjectCompleted;

//! add an object to the input queue of an input port
class V_COREEXPORT AddObject: public MessageBase<AddObject, Message::ADDOBJECT> {

 public:
   AddObject(const std::string &senderPort, vistle::Object::const_ptr obj,
         const std::string &destPort = "");
   AddObject(const AddObject &other);
   AddObject(const AddObjectCompleted &complete);
   ~AddObject();

   void setSenderPort(const std::string &sendPort);
   const char * getSenderPort() const;
   void setDestPort(const std::string &destPort);
   const char * getDestPort() const;
   const char *objectName() const;
   Object::const_ptr takeObject() const; //!< may only be called once
   Object::const_ptr getObject() const; //! try to retrieve from shmem by name
   bool ref() const; //!< may only be called once
   const Meta &meta() const;
   Object::Type objectType() const;
   const shm_handle_t &getHandle() const;
   bool handleValid() const;

 private:
   port_name_t senderPort;
   port_name_t destPort;
   shm_name_t m_name;
   shmsegname_t m_shmname;
   Meta m_meta;
   int m_objectType;
   const shm_handle_t handle;
   mutable bool m_handleValid;
};
BOOST_STATIC_ASSERT(sizeof(AddObject) <= Message::MESSAGE_SIZE);

class V_COREEXPORT AddObjectCompleted: public MessageBase<AddObjectCompleted, Message::ADDOBJECTCOMPLETED> {

 public:
   AddObjectCompleted(const AddObject &msg);
   const char *objectName() const;
   int originalDestination() const;

 private:
   shm_name_t m_name;
   int m_orgDestId;
};
BOOST_STATIC_ASSERT(sizeof(AddObjectCompleted) <= Message::MESSAGE_SIZE);

//! notify rank 0 controller that an object was received
class V_COREEXPORT ObjectReceived: public MessageBase<ObjectReceived, Message::OBJECTRECEIVED> {

 public:
   ObjectReceived(const AddObject &add, const std::string &destPort="");
   const char * getSenderPort() const;
   const char *getDestPort() const;
   const char *objectName() const;
   const Meta &meta() const;
   Object::Type objectType() const;

 private:
   port_name_t senderPort;
   port_name_t portName;
   shm_name_t m_name;
   Meta m_meta;
   int m_objectType;
};
BOOST_STATIC_ASSERT(sizeof(ObjectReceived) <= Message::MESSAGE_SIZE);

//! connect an output port to an input port of another module
class V_COREEXPORT Connect: public MessageBase<Connect, Message::CONNECT> {

 public:
   Connect(const int moduleIDA, const std::string & portA,
           const int moduleIDB, const std::string & portB);

   const char * getPortAName() const;
   const char * getPortBName() const;

   int getModuleA() const;
   int getModuleB() const;

   void reverse(); //! swap source and destination

 private:
   port_name_t portAName;
   port_name_t portBName;

   int moduleA;
   int moduleB;
};
BOOST_STATIC_ASSERT(sizeof(Connect) <= Message::MESSAGE_SIZE);

//! disconnect an output port from an input port of another module
class V_COREEXPORT Disconnect: public MessageBase<Disconnect, Message::DISCONNECT> {

 public:
   Disconnect(const int moduleIDA, const std::string & portA,
           const int moduleIDB, const std::string & portB);

   const char * getPortAName() const;
   const char * getPortBName() const;

   int getModuleA() const;
   int getModuleB() const;

   void reverse(); //! swap source and destination

 private:
   port_name_t portAName;
   port_name_t portBName;

   int moduleA;
   int moduleB;
};
BOOST_STATIC_ASSERT(sizeof(Disconnect) <= Message::MESSAGE_SIZE);

//! notification that a module has created a parameter
class V_COREEXPORT AddParameter: public MessageBase<AddParameter, Message::ADDPARAMETER> {
   public:
      AddParameter(const Parameter &param, const std::string &moduleName);

      const char *getName() const;
      const char *moduleName() const;
      const char *description() const;
      const char *group() const;
      int getParameterType() const;
      int getPresentation() const;
      boost::shared_ptr<Parameter> getParameter() const; //< allocates a new Parameter object, caller is responsible for deletion

   private:
      param_name_t name;
      param_name_t m_group;
      module_name_t module;
      param_desc_t m_description;
      int paramtype;
      int presentation;
};
BOOST_STATIC_ASSERT(sizeof(AddParameter) <= Message::MESSAGE_SIZE);

//! notification that a module has removed a parameter
class V_COREEXPORT RemoveParameter: public MessageBase<RemoveParameter, Message::REMOVEPARAMETER> {
   public:
      RemoveParameter(const Parameter &param, const std::string &moduleName);

      const char *getName() const;
      const char *moduleName() const;
      int getParameterType() const;
      boost::shared_ptr<Parameter> getParameter() const; //< allocates a new Parameter object, caller is responsible for deletion

   private:
      param_name_t name;
      param_name_t m_group;
      module_name_t module;
      int paramtype;
};
BOOST_STATIC_ASSERT(sizeof(RemoveParameter) <= Message::MESSAGE_SIZE);

//! request parameter value update or notify that a parameter value has been changed
class V_COREEXPORT SetParameter: public MessageBase<SetParameter, Message::SETPARAMETER> {
   public:
      SetParameter(int module, const std::string & name, const boost::shared_ptr<Parameter> param, Parameter::RangeType rt=Parameter::Value);
      SetParameter(int module, const std::string & name, const Integer value);
      SetParameter(int module, const std::string & name, const Float value);
      SetParameter(int module, const std::string & name, const ParamVector &value);
      SetParameter(int module, const std::string & name, const IntParamVector &value);
      SetParameter(int module, const std::string & name, const std::string &value);

      void setInit();
      bool isInitialization() const;
      void setModule(int );
      int getModule() const;
      bool setType(int type);

      void setRangeType(int rt);
      int rangeType() const;

      const char * getName() const;
      int getParameterType() const;

      Integer getInteger() const;
      std::string getString() const;
      Float getFloat() const;
      ParamVector getVector() const;
      IntParamVector getIntVector() const;

      bool apply(boost::shared_ptr<Parameter> param) const;

   private:
      int m_module; //!< destination module
      param_name_t name;
      int paramtype;
      int dim;
      bool initialize;
      bool reply;
      int rangetype;
      union {
         Integer v_int;
         Float v_scalar;
         Float v_vector[MaxDimension];
         Integer v_ivector[MaxDimension];
         param_value_t v_string;
      };
};
BOOST_STATIC_ASSERT(sizeof(SetParameter) <= Message::MESSAGE_SIZE);

//! set list of choice descriptions for a choice parameter
class V_COREEXPORT SetParameterChoices: public MessageBase<SetParameterChoices, Message::SETPARAMETERCHOICES> {
   public:
      SetParameterChoices(const std::string &name, const std::vector<std::string> &choices);

      const char *getName() const;
      int getNumChoices() const;
      const char *getChoice(int idx) const;

      bool apply(boost::shared_ptr<Parameter> param) const;

   private:
      int numChoices;
      param_name_t name;
      param_choice_t choices[param_num_choices];
};
BOOST_STATIC_ASSERT(sizeof(SetParameterChoices) <= Message::MESSAGE_SIZE);

class V_COREEXPORT Barrier: public MessageBase<Barrier, Message::BARRIER> {

 public:
   Barrier();
};
BOOST_STATIC_ASSERT(sizeof(Barrier) <= Message::MESSAGE_SIZE);

class V_COREEXPORT BarrierReached: public MessageBase<BarrierReached, Message::BARRIERREACHED> {

 public:
   BarrierReached(const uuid_t &uuid);
};
BOOST_STATIC_ASSERT(sizeof(BarrierReached) <= Message::MESSAGE_SIZE);

class V_COREEXPORT SetId: public MessageBase<SetId, Message::SETID> {

 public:
   SetId(const int id);

   int getId() const;

 private:
   const int m_id;
};
BOOST_STATIC_ASSERT(sizeof(SetId) <= Message::MESSAGE_SIZE);

class V_COREEXPORT ReplayFinished: public MessageBase<ReplayFinished, Message::REPLAYFINISHED> {

public:
   ReplayFinished();
};
BOOST_STATIC_ASSERT(sizeof(ReplayFinished) <= Message::MESSAGE_SIZE);

//! send text messages to user interfaces
class V_COREEXPORT SendText: public MessageBase<SendText, Message::SENDTEXT> {

public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(TextType,
      (Cout)
      (Cerr)
      (Clog)
      (Info)
      (Warning)
      (Error)
   )

   //! Error message in response to a Message
   SendText(const std::string &text, const Message &inResponseTo);
   SendText(TextType type, const std::string &text);

   TextType textType() const;
   Type referenceType() const;
   uuid_t referenceUuid() const;
   const char *text() const;
   bool truncated() const;

private:
   //! type of text
   TextType m_textType;
   //! uuid of Message this message is a response to
   uuid_t m_referenceUuid;
   //! Type of Message this message is a response to
   Type m_referenceType;
   //! message text
   text_t m_text;
   //! whether m_text has been truncated
   bool m_truncated;
};
BOOST_STATIC_ASSERT(sizeof(SendText) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(TextType, SendText)

class V_COREEXPORT ObjectReceivePolicy: public MessageBase<ObjectReceivePolicy, Message::OBJECTRECEIVEPOLICY> {

public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Policy,
      (Single)
      (NotifyAll)
      (Distribute)
   )
   ObjectReceivePolicy(Policy pol);
   Policy policy() const;
private:
   Policy m_policy;
};
BOOST_STATIC_ASSERT(sizeof(ObjectReceivePolicy) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(Policy, ObjectReceivePolicy)

class V_COREEXPORT SchedulingPolicy: public MessageBase<SchedulingPolicy, Message::SCHEDULINGPOLICY> {

public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Schedule,
      (Ignore) //< prepare/compute/reduce not called at all (e.g. for renderers)
      (Single) //< compute called on each rank individually once per received object
      (Gang) //< compute called on all ranks together once per received object
      (LazyGang) //< compute called on all ranks together, but not necessarily for each received object
   )
   SchedulingPolicy(Schedule pol);
   Schedule policy() const;
private:
   Schedule m_policy;
};
BOOST_STATIC_ASSERT(sizeof(SchedulingPolicy) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(Schedule, SchedulingPolicy)

//! control whether/when prepare() and reduce() are called
class V_COREEXPORT ReducePolicy: public MessageBase<ReducePolicy, Message::REDUCEPOLICY> {

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Reduce,
      (Never) //< module's prepare()/reduce() methods will never be called - only for modules with COMBINE port (renderers)
      (Locally) //< module's prepare()/reduce() methods will be called unsynchronized on each rank
      //(PerTimestep) //< module's reduce() method will be called on all ranks together once per timestep
      (OverAll) //< module's prepare()/reduce() method will be called on all ranks together after all timesteps have been received
   )
   ReducePolicy(Reduce red);
   Reduce policy() const;
 private:
   Reduce m_reduce;
};
BOOST_STATIC_ASSERT(sizeof(ReducePolicy) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(Reduce, ReducePolicy)

//! steer execution stages
/*!
 * module ranks notify the cluster manager about their execution stage
 *
 *
 */
class V_COREEXPORT ExecutionProgress: public MessageBase<ExecutionProgress, Message::EXECUTIONPROGRESS> {

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Progress,
      (Start) //< execution starts - if applicable, prepare() will be invoked
      (Finish) //< execution finishes - if applicable, reduce() has finished
   )
   ExecutionProgress(Progress stage);
   Progress stage() const;
   void setStage(Progress stage);

 private:
   Progress m_stage;
};
BOOST_STATIC_ASSERT(sizeof(ExecutionProgress) <= Message::MESSAGE_SIZE);
V_ENUM_OUTPUT_OP(Progress, ExecutionProgress)

//! enable/disable message tracing for a module
class V_COREEXPORT Trace: public MessageBase<Trace, Message::TRACE> {

 public:
   Trace(int module, Message::Type type, bool onoff);
   int module() const;
   Type messageType() const;
   bool on() const;

 private:
   int m_module;
   Type m_messageType;
   bool m_on;
};
BOOST_STATIC_ASSERT(sizeof(Trace) <= Message::MESSAGE_SIZE);

//! announce availability of a module to UI
class V_COREEXPORT ModuleAvailable: public MessageBase<ModuleAvailable, Message::MODULEAVAILABLE> {

 public:
   ModuleAvailable(int hub, const std::string &name, const std::string &path = std::string());
   const char *name() const;
   const char *path() const;
   int hub() const;

 private:
   int m_hub;
   module_name_t m_name;
   text_t m_path;
};
BOOST_STATIC_ASSERT(sizeof(ModuleAvailable) <= Message::MESSAGE_SIZE);

//! lock UI (block user interaction)
class V_COREEXPORT LockUi: public MessageBase<LockUi, Message::LOCKUI> {

 public:
   LockUi(bool locked);
   bool locked() const;

 private:
   bool m_locked;
};
BOOST_STATIC_ASSERT(sizeof(LockUi) <= Message::MESSAGE_SIZE);

//! request hub to listen on TCP port and forward incoming connections
class V_COREEXPORT RequestTunnel: public MessageBase<RequestTunnel, Message::REQUESTTUNNEL> {

 public:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(AddressType,
      (Hostname)
      (IPv4)
      (IPv6)
      (Unspecified)
   )
   //! establish tunnel - let hub forward incoming connections to srcPort to destHost::destPort
   RequestTunnel(unsigned short srcPort, const std::string &destHost, unsigned short destPort);
   //! establish tunnel - let hub forward incoming connections to srcPort to destHost::destPort
   RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v4 destHost, unsigned short destPort);
   //! establish tunnel - let hub forward incoming connections to srcPort to destHost::destPort
   RequestTunnel(unsigned short srcPort, const boost::asio::ip::address_v6 destHost, unsigned short destPort);
   //! establish tunnel - let hub forward incoming connections to srcPort to destPort on local interface, address will be filled in by rank 0 of cluster manager
   RequestTunnel(unsigned short srcPort, unsigned short destPort);
   //! remove tunnel
   RequestTunnel(unsigned short srcPort);

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
   text_t m_destAddr;
   unsigned short m_destPort;
   bool m_remove;
};
BOOST_STATIC_ASSERT(sizeof(RequestTunnel) <= Message::MESSAGE_SIZE);

//! request remote data object
class V_COREEXPORT RequestObject: public MessageBase<RequestObject, Message::REQUESTOBJECT> {

 public:
   RequestObject(const AddObject &add, const std::string &objId, const std::string &referrer="");
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
BOOST_STATIC_ASSERT(sizeof(RequestObject) <= Message::MESSAGE_SIZE);

//! header for data object transmission
class V_COREEXPORT SendObject: public MessageBase<SendObject, Message::SENDOBJECT> {

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
BOOST_STATIC_ASSERT(sizeof(RequestObject) <= Message::MESSAGE_SIZE);

V_COREEXPORT std::ostream &operator<<(std::ostream &s, const Message &msg);

} // namespace message
} // namespace vistle

#pragma pack(pop)
#endif
