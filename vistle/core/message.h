#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "object.h"
#include "vector.h"

namespace vistle {

class Communicator;

namespace message {

typedef char module_name_t[32];
typedef char port_name_t[32];
typedef char param_name_t[32];
typedef char param_value_t[256];

struct Message {
   // this is POD

   friend class vistle::Communicator;

 public:
   static const size_t MESSAGE_SIZE = 512;

   enum Type {
      DEBUG              =   0,
      SPAWN              =   1,
      KILL               =   2,
      QUIT               =   3,
      NEWOBJECT          =   4,
      MODULEEXIT         =   5,
      COMPUTE            =   6,
      CREATEINPUTPORT    =   7,
      CREATEOUTPUTPORT   =   8,
      ADDOBJECT          =   9,
      CONNECT            =  10,
      ADDFILEPARAMETER   =  11,
      SETFILEPARAMETER   =  12,
      ADDFLOATPARAMETER  =  13,
      SETFLOATPARAMETER  =  14,
      ADDINTPARAMETER    =  15,
      SETINTPARAMETER    =  16,
      ADDVECTORPARAMETER =  17,
      SETVECTORPARAMETER =  18,
      PING               =  19,
      PONG               =  20,
      BUSY               =  21,
      IDLE               =  22,
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
class Ping: public Message {

 public:
   Ping(const int moduleID, const int rank, const char c);

   char getCharacter() const;

 private:
   const char character;
};
BOOST_STATIC_ASSERT(sizeof(Ping) < Message::MESSAGE_SIZE);

//! debug: reply to pong
class Pong: public Message {

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
class Spawn: public Message {

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

//! request a module to quit
class Kill: public Message {

 public:
   Kill(const int moduleID, const int rank, const int module);

   int getModule() const;

 private:
   //! ID of module to stop
   const int module;
};
BOOST_STATIC_ASSERT(sizeof(Kill) < Message::MESSAGE_SIZE);

//! request all modules to quit for terminating the session
class Quit: public Message {

 public:
   Quit(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(Quit) < Message::MESSAGE_SIZE);

class NewObject: public Message {

 public:
   NewObject(const int moduleID, const int rank,
             const shm_handle_t & handle);

   const shm_handle_t & getHandle() const;

 private:
   shm_handle_t handle;
};
BOOST_STATIC_ASSERT(sizeof(NewObject) < Message::MESSAGE_SIZE);

class ModuleExit: public Message {

 public:
   ModuleExit(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(ModuleExit) < Message::MESSAGE_SIZE);

//! trigger computation for a module
class Compute: public Message {

 public:
   Compute(const int moduleID, const int rank, const int module);

   int getModule() const;

 private:
   const int module;
};
BOOST_STATIC_ASSERT(sizeof(Compute) < Message::MESSAGE_SIZE);

//! indicate that a module has started computing
class Busy: public Message {

 public:
   Busy(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(Busy) < Message::MESSAGE_SIZE);

//! indicate that a module has finished computing
class Idle: public Message {

 public:
   Idle(const int moduleID, const int rank);

 private:
};
BOOST_STATIC_ASSERT(sizeof(Idle) < Message::MESSAGE_SIZE);

class CreateInputPort: public Message {

 public:
   CreateInputPort(const int moduleID, const int rank,
                   const std::string & name);

   const char * getName() const;

 private:
   port_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(CreateInputPort) < Message::MESSAGE_SIZE);

class CreateOutputPort: public Message {

 public:
   CreateOutputPort(const int moduleID, const int rank,
                    const std::string & name);

   const char * getName() const;

 private:
   port_name_t name;
};
BOOST_STATIC_ASSERT(sizeof(CreateOutputPort) < Message::MESSAGE_SIZE);

//! add an object to the input queue of an input port
class AddObject: public Message {

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
class Connect: public Message {

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

class AddFileParameter: public Message {

 public:
   AddFileParameter(const int moduleID, const int rank,
                    const std::string & name,
                    const std::string & value);

   const char * getName() const;
   const char * getValue() const;

 private:
   param_name_t name;
   param_value_t value;
};
BOOST_STATIC_ASSERT(sizeof(AddFileParameter) < Message::MESSAGE_SIZE);

class SetFileParameter: public Message {

 public:
   SetFileParameter(const int moduleID, const int rank, const int module,
                    const std::string & name,
                    const std::string & value);

   int getModule() const;
   const char * getName() const;
   const char * getValue() const;

 private:
   const int module;
   param_name_t name;
   char value[256];
};
BOOST_STATIC_ASSERT(sizeof(SetFileParameter) < Message::MESSAGE_SIZE);

class AddFloatParameter: public Message {

 public:
   AddFloatParameter(const int moduleID, const int rank,
                     const std::string & name, const vistle::Scalar value);

   const char * getName() const;
   vistle::Scalar getValue() const;

 private:
   param_name_t name;
   vistle::Scalar value;
};
BOOST_STATIC_ASSERT(sizeof(AddFloatParameter) < Message::MESSAGE_SIZE);

class SetFloatParameter: public Message {

 public:
   SetFloatParameter(const int moduleID, const int rank, const int module,
                     const std::string & name, const vistle::Scalar value);

   int getModule() const;
   const char * getName() const;
   vistle::Scalar getValue() const;

 private:
   const int module;
   param_name_t name;
   vistle::Scalar value;
};
BOOST_STATIC_ASSERT(sizeof(SetFloatParameter) < Message::MESSAGE_SIZE);

class AddIntParameter: public Message {

 public:
   AddIntParameter(const int moduleID, const int rank,
                   const std::string & name, const int value);

   const char * getName() const;
   int getValue() const;

 private:
   param_name_t name;
   int value;
};
BOOST_STATIC_ASSERT(sizeof(AddIntParameter) < Message::MESSAGE_SIZE);

class SetIntParameter: public Message {

 public:
   SetIntParameter(const int moduleID, const int rank, const int module,
                   const std::string & name, const int value);

   int getModule() const;
   const char * getName() const;
   int getValue() const;

 private:
   const int module;
   param_name_t name;
   int value;
};
BOOST_STATIC_ASSERT(sizeof(SetIntParameter) < Message::MESSAGE_SIZE);

class AddVectorParameter: public Message {

 public:
   AddVectorParameter(const int moduleID, const int rank,
                      const std::string & name, const Vector & value);

   const char * getName() const;
   Vector getValue() const;

 private:
   param_name_t name;
   Vector value;
};
BOOST_STATIC_ASSERT(sizeof(AddVectorParameter) < Message::MESSAGE_SIZE);

class SetVectorParameter: public Message {

 public:
   SetVectorParameter(const int moduleID, const int rank, const int module,
                      const std::string & name, const Vector & value);

   int getModule() const;
   const char * getName() const;
   Vector getValue() const;

 private:
   const int module;
   param_name_t name;
   Vector value;
};
BOOST_STATIC_ASSERT(sizeof(SetVectorParameter) < Message::MESSAGE_SIZE);

} // namespace message
} // namespace vistle

#endif
