#ifndef MESSAGE_H
#define MESSAGE_H

#include <boost/interprocess/managed_shared_memory.hpp>
#include <string>

namespace vistle {

typedef boost::interprocess::managed_shared_memory::handle_t shm_handle_t;

class Communicator;

namespace message {

class Message {

   friend class vistle::Communicator;

 public:
   enum Type {
      DEBUG            =  0,
      SPAWN            =  1,
      QUIT             =  2,
      NEWOBJECT        =  3,
      MODULEEXIT       =  4,
      COMPUTE          =  5,
      CREATEINPUTPORT  =  6,
      CREATEOUTPUTPORT =  7,
      ADDOBJECT        =  8,
      CONNECT          =  9,
   };

   Message(const int moduleID, const int rank,
           const Type type, const unsigned int size);

   Type getType() const;
   int getModuleID() const;
   int getRank() const;

 private:
   const int moduleID;
   const int rank;
   unsigned int size;
   const Type type;
};

class Debug: public Message {

 public:
   Debug(const int moduleID, const int rank, const char c);

   char getCharacter() const;

 private:
   const char character;
};

class Spawn: public Message {

 public:
   Spawn(const int moduleID, const int rank, const int spawnID,
         const std::string &name);

   int getSpawnID() const;
   const char *getName() const;

 private:
   const int spawnID;
   char name[32];
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
   char name[32];
};

class CreateOutputPort: public Message {

 public:
   CreateOutputPort(const int moduleID, const int rank,
                    const std::string & name);

   const char * getName() const;

 private:
   char name[32];
};

class AddObject: public Message {

 public:
   AddObject(const int moduleID, const int rank, const std::string & portName,
             const shm_handle_t & handle);

   const char * getPortName() const;
   const shm_handle_t & getHandle() const;

 private:
   char portName[32];
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
   char portAName[32];
   char portBName[32];

   const int moduleA;
   const int moduleB;
};

} // namespace message
} // namespace vistle

#endif
