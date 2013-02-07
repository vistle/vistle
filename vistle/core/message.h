#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "object.h"
#include "paramvector.h"
#include "export.h"

namespace vistle {

class Communicator;
class Parameter;

namespace message {

typedef char module_name_t[32];
typedef char port_name_t[32];
typedef char param_name_t[32];
typedef char param_value_t[256];

struct V_COREEXPORT Message {
   // this is POD

   friend class vistle::Communicator;

 public:
   static const size_t MESSAGE_SIZE = 512;

   enum Type {
      DEBUG,
      SPAWN,
      STARTED,
      KILL,
      QUIT,
      NEWOBJECT,
      MODULEEXIT,
      COMPUTE,
      CREATEINPUTPORT,
      CREATEOUTPUTPORT,
      ADDOBJECT,
      CONNECT,
      ADDPARAMETER,
      SETPARAMETER,
      PING,
      PONG,
      BUSY,
      IDLE,
      BARRIER,
      BARRIERREACHED,
   };

   Message(const int moduleID, const int rank,
           const Type type, const unsigned int size);
   // Message (or it's subclasses) may not require desctructors

   //! message type
   Type getType() const;
   //! sender ID
   int getModuleID() const;
   //! sender rank
   int getRank() const;
   //! messge size
   size_t getSize() const;

 private:
   //! message size
   unsigned int size;
   //! message type
   const Type type;
   //! sender ID
   const int moduleID;
   //! sender rank
   const int rank;
};

//! debug: request a reply containing character 'c'
class V_COREEXPORT Ping: public Message {

 public:
   Ping(const int moduleID, const int rank, const char c);

   char getCharacter() const;

 private:
   const char character;
};
BOOST_STATIC_ASSERT(sizeof(Ping) < Message::MESSAGE_SIZE);

//! debug: reply to pong
class V_COREEXPORT Pong: public Message {

 public:
   Pong(const int moduleID, const int rank, const char c, const int module);

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
   Spawn(const int moduleID, const int rank, const int spawnID,
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
   Started(const int moduleID, const int rank, const std::string &name);

   const char *getName() const;

 private:
   //! name of module to be started
   module_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(Started) < Message::MESSAGE_SIZE);

//! request a module to quit
class V_COREEXPORT Kill: public Message {

 public:
   Kill(const int moduleID, const int rank, const int module);

   int getModule() const;

 private:
   //! ID of module to stop
   const int module;
};
BOOST_STATIC_ASSERT(sizeof(Kill) < Message::MESSAGE_SIZE);

//! request all modules to quit for terminating the session
class V_COREEXPORT Quit: public Message {

 public:
   Quit(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(Quit) < Message::MESSAGE_SIZE);

class V_COREEXPORT NewObject: public Message {

 public:
   NewObject(const int moduleID, const int rank,
             const shm_handle_t & handle);

   const shm_handle_t & getHandle() const;

 private:
   shm_handle_t handle;
};
BOOST_STATIC_ASSERT(sizeof(NewObject) < Message::MESSAGE_SIZE);

class V_COREEXPORT ModuleExit: public Message {

 public:
   ModuleExit(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(ModuleExit) < Message::MESSAGE_SIZE);

//! trigger computation for a module
class V_COREEXPORT Compute: public Message {

 public:
   Compute(const int moduleID, const int rank, const int module);

   int getModule() const;

 private:
   const int module;
};
BOOST_STATIC_ASSERT(sizeof(Compute) < Message::MESSAGE_SIZE);

//! indicate that a module has started computing
class V_COREEXPORT Busy: public Message {

 public:
   Busy(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(Busy) < Message::MESSAGE_SIZE);

//! indicate that a module has finished computing
class V_COREEXPORT Idle: public Message {

 public:
   Idle(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(Idle) < Message::MESSAGE_SIZE);

class V_COREEXPORT CreateInputPort: public Message {

 public:
   CreateInputPort(const int moduleID, const int rank,
                   const std::string & name);

   const char * getName() const;

 private:
   port_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(CreateInputPort) < Message::MESSAGE_SIZE);

class V_COREEXPORT CreateOutputPort: public Message {

 public:
   CreateOutputPort(const int moduleID, const int rank,
                    const std::string & name);

   const char * getName() const;

 private:
   port_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(CreateOutputPort) < Message::MESSAGE_SIZE);

//! add an object to the input queue of an input port
class V_COREEXPORT AddObject: public Message {

 public:
   AddObject(const int moduleID, const int rank, const std::string & portName,
             vistle::Object::const_ptr obj);

   const char * getPortName() const;
   const shm_handle_t & getHandle() const;
   Object::const_ptr takeObject() const;

 private:
   port_name_t portName;
   const shm_handle_t handle;
};
BOOST_STATIC_ASSERT(sizeof(AddObject) < Message::MESSAGE_SIZE);

//! connect an output port to an input port of another module
class V_COREEXPORT Connect: public Message {

 public:
   Connect(const int moduleID, const int rank,
           const int moduleIDA, const std::string & portA,
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

class V_COREEXPORT AddParameter: public Message {
   public:
      AddParameter(const int moduleID, const int rank,
            const std::string & name, int type);

      const char * getName() const;
      int getParameterType() const;
      Parameter *getParameter() const; // allocates a new Parameter object, caller is responsible for deletion

   private:
      param_name_t name;
      int paramtype;
};
BOOST_STATIC_ASSERT(sizeof(AddParameter) < Message::MESSAGE_SIZE);

class V_COREEXPORT SetParameter: public Message {
   public:
      SetParameter(const int moduleID, const int rank, const int module,
            const std::string & name, const Parameter *param);
      SetParameter(const int moduleID, const int rank, const int module,
            const std::string & name, const int value);
      SetParameter(const int moduleID, const int rank, const int module,
            const std::string & name, const Scalar value);
      SetParameter(const int moduleID, const int rank, const int module,
            const std::string & name, const ParamVector value);
      SetParameter(const int moduleID, const int rank, const int module,
            const std::string & name, const std::string &value);

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
      union {
         int v_int;
         Scalar v_scalar;
         Scalar v_vector[MaxDimension];
         param_value_t v_string;
      };
};
BOOST_STATIC_ASSERT(sizeof(SetParameter) < Message::MESSAGE_SIZE);

class V_COREEXPORT Barrier: public Message {

 public:
   Barrier(const int moduleID, const int rank, const int id);

   int getBarrierId() const;

 private:
   const int barrierid;
};
BOOST_STATIC_ASSERT(sizeof(Barrier) < Message::MESSAGE_SIZE);

class V_COREEXPORT BarrierReached: public Message {

 public:
   BarrierReached(const int moduleID, const int rank, const int id);

   int getBarrierId() const;

 private:
   const int barrierid;
};
BOOST_STATIC_ASSERT(sizeof(BarrierReached) < Message::MESSAGE_SIZE);
} // namespace message
} // namespace vistle

#endif
