#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "object.h"
#include "paramvector.h"
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
typedef char param_choice_t[64];
const int param_num_choices = 60;

struct V_COREEXPORT Message {
   // this is POD

   friend class vistle::Communicator;

 public:
   static const size_t MESSAGE_SIZE = 4096; // fixed message size is imposed by boost::interprocess::message_queue

   enum Type {
      DEBUG,
      SPAWN,
      STARTED,
      KILL,
      QUIT,
      NEWOBJECT,
      MODULEEXIT,
      COMPUTE,
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
   };

   Message(const Type type, const unsigned int size);
   // Message (or its subclasses) may not require destructors

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

 private:
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
         const std::string &name, int debugFlag = 0, int debugRank = 0);

   int getSpawnID() const;
   const char *getName() const;
   int getDebugFlag() const;
   int getDebugRank() const;

 private:
   //! ID of module to spawn
   const int spawnID;
   //! start with debugger/memory tracer
   const int debugFlag;
   //! on which rank to attach debugger
   const int debugRank;
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

class V_COREEXPORT NewObject: public Message {

 public:
   NewObject(const shm_handle_t &handle);

   const shm_handle_t & getHandle() const;

 private:
   shm_handle_t handle;
};
BOOST_STATIC_ASSERT(sizeof(NewObject) < Message::MESSAGE_SIZE);

class V_COREEXPORT ModuleExit: public Message {

 public:
   ModuleExit();

 private:
};
BOOST_STATIC_ASSERT(sizeof(ModuleExit) < Message::MESSAGE_SIZE);

//! trigger computation for a module
class V_COREEXPORT Compute: public Message {

 public:
   Compute(const int module, const int count);

   int getModule() const;
   int getExecutionCount() const;

 private:
   const int module;
   const int executionCount;
};
BOOST_STATIC_ASSERT(sizeof(Compute) < Message::MESSAGE_SIZE);

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

 private:
   port_name_t portName;
   shm_name_t m_name;
   Meta m_meta;
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
      AddParameter(const std::string &name, const std::string &description, int type, int presentation, const std::string &moduleName);

      const char *getName() const;
      const char *moduleName() const;
      const char *description() const;
      int getParameterType() const;
      int getPresentation() const;
      Parameter *getParameter() const; //< allocates a new Parameter object, caller is responsible for deletion

   private:
      param_name_t name;
      module_name_t module;
      param_desc_t m_description;
      int paramtype;
      int presentation;
};
BOOST_STATIC_ASSERT(sizeof(AddParameter) < Message::MESSAGE_SIZE);

class V_COREEXPORT SetParameter: public Message {
   public:
      enum RangeType {
         Value,
         Minimum,
         Maximum
      };
      SetParameter(const int module,
            const std::string & name, const Parameter *param);
      SetParameter(const int module,
            const std::string & name, const int value);
      SetParameter(const int module,
            const std::string & name, const Scalar value);
      SetParameter(const int module,
            const std::string & name, const ParamVector value);
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

      int getInteger() const;
      std::string getString() const;
      Scalar getScalar() const;
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
         int v_int;
         Scalar v_scalar;
         Scalar v_vector[MaxDimension];
         param_value_t v_string;
      };
};
BOOST_STATIC_ASSERT(sizeof(SetParameter) < Message::MESSAGE_SIZE);

class V_COREEXPORT SetParameterChoices: public Message {
   public:
      SetParameterChoices(const int module,
            const std::string &name, const std::vector<std::string> &choices);

      int getModule() const;
      const char * getName() const;
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
} // namespace message
} // namespace vistle

#endif
