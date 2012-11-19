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
   };

   Message(const int moduleID, const int rank,
           const Type type, const unsigned int size);
   // Message (or it's subclasses may not require desctructors

   Type getType() const;
   int getModuleID() const;
   int getRank() const;
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

class Ping: public Message {

 public:
   Ping(const int moduleID, const int rank, const char c);

   char getCharacter() const;

 private:
   const char character;
};

class Pong: public Message {

 public:
   Pong(const int moduleID, const int rank, const char c, const int module);

   char getCharacter() const;
   int getDestination() const;

 private:
   const char character;
   int module;
};

class Spawn: public Message {

 public:
   Spawn(const int moduleID, const int rank, const int spawnID,
         const std::string &name, int debugFlag = 0, int debugRank = 0);

   int getSpawnID() const;
   const char *getName() const;
   int getDebugFlag() const;
   int getDebugRank() const;

 private:
   const int spawnID;
   const int debugFlag;
   const int debugRank;
   module_name_t name;
};

class Kill: public Message {

 public:
   Kill(const int moduleID, const int rank, const int module);

   int getModule() const;

 private:
   const int module;
};

class Quit: public Message {

 public:
   Quit(const int moduleID, const int rank);

 private:
};

class NewObject: public Message {

 public:
   NewObject(const int moduleID, const int rank,
             const shm_handle_t & handle);

   const shm_handle_t & getHandle() const;

 private:
   shm_handle_t handle;
};

class ModuleExit: public Message {

 public:
   ModuleExit(const int moduleID, const int rank);

 private:
};

class Compute: public Message {

 public:
   Compute(const int moduleID, const int rank, const int module);

   int getModule() const;

 private:
   const int module;
};

class CreateInputPort: public Message {

 public:
   CreateInputPort(const int moduleID, const int rank,
                   const std::string & name);

   const char * getName() const;

 private:
   port_name_t name;
};

class CreateOutputPort: public Message {

 public:
   CreateOutputPort(const int moduleID, const int rank,
                    const std::string & name);

   const char * getName() const;

 private:
   port_name_t name;
};

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

} // namespace message
} // namespace vistle

#endif
