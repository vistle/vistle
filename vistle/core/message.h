#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <boost/uuid/uuid.hpp>


#include "object.h"
#include "scalar.h"
#include "paramvector.h"
#include "parameter.h"
#include "export.h"

namespace vistle {

class Communicator;
class Parameter;
class Port;

namespace message {

class V_COREEXPORT DefaultSender {

   public:
      DefaultSender();
      static void init(int id, int rank);
      static const DefaultSender &instance();
      static int id();
      static int rank();

   private:
      int m_id;
      int m_rank;
      static DefaultSender s_instance;
};

typedef char module_name_t[32];
typedef char port_name_t[32];
typedef char param_name_t[32];
typedef char param_value_t[256];
typedef char param_desc_t[512];
typedef char param_choice_t[40];
const int param_num_choices = 22;
typedef char text_t[900];

class V_COREEXPORT Message {
   // this is POD

   friend class vistle::Communicator;

 public:
   static const size_t MESSAGE_SIZE = 1024; // fixed message size is imposed by boost::interprocess::message_queue
   typedef boost::uuids::uuid uuid_t;

   enum Type {
      INVALID = 0,
      DEBUG,
      SPAWN,
      STARTED,
      KILL,
      QUIT,
      MODULEEXIT,
      COMPUTE,
      REDUCE,
      CREATEPORT,
      ADDOBJECT,
      OBJECTRECEIVED,
      CONNECT,
      DISCONNECT,
      ADDPARAMETER,
      SETPARAMETER,
      SETPARAMETERCHOICES,
      PING,
      PONG,
      BUSY,
      IDLE,
      BARRIER,
      BARRIERREACHED,
      SETID,
      RESETMODULEIDS,
      REPLAYFINISHED,
      SENDTEXT,
      OBJECTRECEIVEPOLICY,
      SCHEDULINGPOLICY,
      REDUCEPOLICY,
      EXECUTIONPROGRESS,
   };

   Message(const Type type, const unsigned int size);
   // Message (or its subclasses) may not require destructors

   //! message uuid - copied to related messages (i.e. responses or errors)
   const uuid_t &uuid() const;
   //! set message uuid
   void setUuid(const uuid_t &uuid);
   //! message type
   Type type() const;
   //! sender ID
   int senderId() const;
   //! set sender ID
   void setSenderId(int id);
   //! sender rank
   int rank() const;
   //! set sender rank
   void setRank(int rank);
   //! messge size
   size_t size() const;
   //! broadacast to all ranks?
   bool broadcast() const;

  protected:
   //! broadcast to all ranks?
   bool m_broadcast;
 private:
   //! message uuid
   uuid_t m_uuid;
   //! message size
   unsigned int m_size;
   //! message type
   const Type m_type;
   //! sender ID
   int m_senderId;
   //! sender rank
   int m_rank;
};

//! debug: request a reply containing character 'c'
class V_COREEXPORT Ping: public Message {

 public:
   Ping(const char c);

   char getCharacter() const;

 private:
   const char character;
};
BOOST_STATIC_ASSERT(sizeof(Ping) < Message::MESSAGE_SIZE);

//! debug: reply to pong
class V_COREEXPORT Pong: public Message {

 public:
   Pong(const char c, const int module);

   char getCharacter() const;
   int getDestination() const;

 private:
   const char character;
   int module;
};
BOOST_STATIC_ASSERT(sizeof(Pong) < Message::MESSAGE_SIZE);

//! spawn a module
class V_COREEXPORT Spawn: public Message {

 public:
   Spawn(const int spawnID,
         const std::string &name, int size=-1, int baserank=-1, int rankskip=-1);

   int spawnId() const;
   void setSpawnId(int id);
   const char *getName() const;
   int getMpiSize() const;
   int getBaseRank() const;
   int getRankSkip() const;

 private:
   //! ID of module to spawn
   int spawnID;
   //! number of ranks in communicator
   int mpiSize;
   //! first rank on which to spawn process
   int baseRank;
   //! number of ranks to skip when spawning process
   int rankSkip;
   //! name of module to be started
   module_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(Spawn) < Message::MESSAGE_SIZE);

//! acknowledge that a module has been spawned
class V_COREEXPORT Started: public Message {

 public:
   Started(const std::string &name);

   const char *getName() const;

 private:
   //! name of module to be started
   module_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(Started) < Message::MESSAGE_SIZE);

//! request a module to quit
class V_COREEXPORT Kill: public Message {

 public:
   Kill(const int module);

   int getModule() const;

 private:
   //! ID of module to stop
   const int module;
};
BOOST_STATIC_ASSERT(sizeof(Kill) < Message::MESSAGE_SIZE);

//! request all modules to quit for terminating the session
class V_COREEXPORT Quit: public Message {

 public:
   Quit();

 private:
};
BOOST_STATIC_ASSERT(sizeof(Quit) < Message::MESSAGE_SIZE);

class V_COREEXPORT ModuleExit: public Message {

 public:
   ModuleExit();
   void setForwarded();
   bool isForwarded() const;

 private:
   bool forwarded;
};
BOOST_STATIC_ASSERT(sizeof(ModuleExit) < Message::MESSAGE_SIZE);

//! trigger computation for a module
class V_COREEXPORT Compute: public Message {

 public:
   Compute(const int module, const int count);

   void setModule(int );
   int getModule() const;
   void setExecutionCount(int count);
   int getExecutionCount() const;

   bool allRanks() const;
   void setAllRanks(bool allRanks);

   enum Reason {
      Execute,
      AddObject,
   };
   Reason reason() const;
   void setReason(Reason r);

private:
   bool m_allRanks; //! whether compute should be broadcasted across all MPI ranks
   int module; //! destination module, -1: all sources
   int executionCount;
   Reason m_reason; //! reason why this message was generated
};
BOOST_STATIC_ASSERT(sizeof(Compute) < Message::MESSAGE_SIZE);

//! trigger reduce() for a module
class V_COREEXPORT Reduce: public Message {

 public:
   Reduce(int module, int timestep=-1);
   int module() const;
   int timestep() const;

 private:
   int m_module;
   int m_timestep;
};
BOOST_STATIC_ASSERT(sizeof(Reduce) < Message::MESSAGE_SIZE);

//! indicate that a module has started computing
class V_COREEXPORT Busy: public Message {

 public:
   Busy();

 private:
};
BOOST_STATIC_ASSERT(sizeof(Busy) < Message::MESSAGE_SIZE);

//! indicate that a module has finished computing
class V_COREEXPORT Idle: public Message {

 public:
   Idle();

 private:
};
BOOST_STATIC_ASSERT(sizeof(Idle) < Message::MESSAGE_SIZE);

class V_COREEXPORT CreatePort: public Message {

 public:
   CreatePort(const Port *port);
   Port *getPort() const;
 private:
   port_name_t m_name;
   int m_porttype;
   int m_flags;
};
BOOST_STATIC_ASSERT(sizeof(CreatePort) < Message::MESSAGE_SIZE);

//! add an object to the input queue of an input port
class V_COREEXPORT AddObject: public Message {

 public:
   AddObject(const std::string & portName,
             vistle::Object::const_ptr obj);

   const char * getPortName() const;
   const shm_handle_t & getHandle() const;
   Object::const_ptr takeObject() const;

 private:
   port_name_t portName;
   const shm_handle_t handle;
};
BOOST_STATIC_ASSERT(sizeof(AddObject) < Message::MESSAGE_SIZE);

//! notify rank 0 controller that an object was received
class V_COREEXPORT ObjectReceived: public Message {

 public:
   ObjectReceived(const std::string &portName,
         vistle::Object::const_ptr obj);
   const char *getPortName() const;
   const char *objectName() const;
   const Meta &meta() const;
   Object::Type objectType() const;

 private:
   port_name_t portName;
   shm_name_t m_name;
   Meta m_meta;
   int m_objectType;
};
BOOST_STATIC_ASSERT(sizeof(ObjectReceived) < Message::MESSAGE_SIZE);

//! connect an output port to an input port of another module
class V_COREEXPORT Connect: public Message {

 public:
   Connect(const int moduleIDA, const std::string & portA,
           const int moduleIDB, const std::string & portB);

   const char * getPortAName() const;
   const char * getPortBName() const;

   int getModuleA() const;
   int getModuleB() const;

 private:
   port_name_t portAName;
   port_name_t portBName;

   const int moduleA;
   const int moduleB;
};
BOOST_STATIC_ASSERT(sizeof(Connect) < Message::MESSAGE_SIZE);

//! disconnect an output port from an input port of another module
class V_COREEXPORT Disconnect: public Message {

 public:
   Disconnect(const int moduleIDA, const std::string & portA,
           const int moduleIDB, const std::string & portB);

   const char * getPortAName() const;
   const char * getPortBName() const;

   int getModuleA() const;
   int getModuleB() const;

 private:
   port_name_t portAName;
   port_name_t portBName;

   const int moduleA;
   const int moduleB;
};
BOOST_STATIC_ASSERT(sizeof(Disconnect) < Message::MESSAGE_SIZE);

class V_COREEXPORT AddParameter: public Message {
   public:
      AddParameter(const Parameter *param, const std::string &moduleName);

      const char *getName() const;
      const char *moduleName() const;
      const char *description() const;
      const char *group() const;
      int getParameterType() const;
      int getPresentation() const;
      Parameter *getParameter() const; //< allocates a new Parameter object, caller is responsible for deletion

   private:
      param_name_t name;
      param_name_t m_group;
      module_name_t module;
      param_desc_t m_description;
      int paramtype;
      int presentation;
};
BOOST_STATIC_ASSERT(sizeof(AddParameter) < Message::MESSAGE_SIZE);

class V_COREEXPORT SetParameter: public Message {
   public:
      SetParameter(const int module,
            const std::string & name, const Parameter *param, Parameter::RangeType rt=Parameter::Value);
      SetParameter(const int module,
            const std::string & name, const Integer value);
      SetParameter(const int module,
            const std::string & name, const Float value);
      SetParameter(const int module,
            const std::string & name, const ParamVector &value);
      SetParameter(const int module,
            const std::string & name, const std::string &value);

      void setInit();
      bool isInitialization() const;
      void setReply();
      bool isReply() const;
      bool setType(int type);

      void setRangeType(int rt);
      int rangeType() const;

      int getModule() const;
      const char * getName() const;
      int getParameterType() const;

      Integer getInteger() const;
      std::string getString() const;
      Float getFloat() const;
      ParamVector getVector() const;

      bool apply(Parameter *param) const;

   private:
      const int module;
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
         param_value_t v_string;
      };
};
BOOST_STATIC_ASSERT(sizeof(SetParameter) < Message::MESSAGE_SIZE);

class V_COREEXPORT SetParameterChoices: public Message {
   public:
      SetParameterChoices(const int module,
            const std::string &name, const std::vector<std::string> &choices);

      int getModule() const;
      const char *getName() const;
      int getNumChoices() const;
      const char *getChoice(int idx) const;

      bool apply(Parameter *param) const;

   private:
      const int module;
      int numChoices;
      param_name_t name;
      param_choice_t choices[param_num_choices];
};
BOOST_STATIC_ASSERT(sizeof(SetParameterChoices) < Message::MESSAGE_SIZE);

class V_COREEXPORT Barrier: public Message {

 public:
   Barrier(const int id);

   int getBarrierId() const;

 private:
   const int barrierid;
};
BOOST_STATIC_ASSERT(sizeof(Barrier) < Message::MESSAGE_SIZE);

class V_COREEXPORT BarrierReached: public Message {

 public:
   BarrierReached(const int id);

   int getBarrierId() const;

 private:
   const int barrierid;
};
BOOST_STATIC_ASSERT(sizeof(BarrierReached) < Message::MESSAGE_SIZE);

class V_COREEXPORT SetId: public Message {

 public:
   SetId(const int id);

   int getId() const;

 private:
   const int m_id;
};
BOOST_STATIC_ASSERT(sizeof(SetId) < Message::MESSAGE_SIZE);

class V_COREEXPORT ResetModuleIds: public Message {

 public:
   ResetModuleIds();
};
BOOST_STATIC_ASSERT(sizeof(ResetModuleIds) < Message::MESSAGE_SIZE);

class V_COREEXPORT ReplayFinished: public Message {

public:
   ReplayFinished();
};
BOOST_STATIC_ASSERT(sizeof(ReplayFinished) < Message::MESSAGE_SIZE);

class V_COREEXPORT SendText: public Message {

public:
   enum TextType {
      Cout,
      Cerr,
      Clog,
      Info,
      Warning,
      Error,
   };

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
BOOST_STATIC_ASSERT(sizeof(SendText) < Message::MESSAGE_SIZE);

class V_COREEXPORT ObjectReceivePolicy: public Message {

public:
   enum Policy {
      Single,
      NotifyAll,
      Distribute,
   };
   ObjectReceivePolicy(Policy pol);
   Policy policy() const;
private:
   Policy m_policy;
};
BOOST_STATIC_ASSERT(sizeof(ObjectReceivePolicy) < Message::MESSAGE_SIZE);

class V_COREEXPORT SchedulingPolicy: public Message {

public:
   enum Schedule {
      Single, //< compute called on each rank individually once per received object
      Gang, //< compute called on all ranks together once per received object
      LazyGang, //< compute called on all ranks together, but not necessarily for each received object
   };
   SchedulingPolicy(Schedule pol);
   Schedule policy() const;
private:
   Schedule m_policy;
};
BOOST_STATIC_ASSERT(sizeof(SchedulingPolicy) < Message::MESSAGE_SIZE);

class V_COREEXPORT ReducePolicy: public Message {

 public:
   enum Reduce {
      Never, //< module's reduce() method will never be called
      PerTimestep, //< module's reduce() method will be called on all ranks together once per timestep
      OverAll, //< module's reduce() method will be called on all ranks together after all timesteps have been received
   };
   ReducePolicy(Reduce red);
   Reduce policy() const;
 private:
   Reduce m_reduce;
};
BOOST_STATIC_ASSERT(sizeof(ReducePolicy) < Message::MESSAGE_SIZE);

class V_COREEXPORT ExecutionProgress: public Message {

 public:
   enum Progress {
      Start,
      Iteration,
      Timestep,
      Finish,
   };
   ExecutionProgress(Progress stage, int step=-1);
   Progress stage() const;
   int step() const;

 private:
   Progress m_stage;
   int m_step;
};
BOOST_STATIC_ASSERT(sizeof(ExecutionProgress) < Message::MESSAGE_SIZE);

} // namespace message
} // namespace vistle

#endif
