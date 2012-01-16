#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <string>

namespace vistle {

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
   Spawn(const int moduleID, const int rank, const int spawnID);

   int getSpawnID() const;

 private:
   const int spawnID;
};

class Quit: public Message {

 public:
   Quit(const int moduleID, const int rank);

 private:
};

class NewObject: public Message {

 public:
   NewObject(const int moduleID, const int rank, const std::string &name);

   const char *getName() const;

 private:
   char name[32];
};

class ModuleExit: public Message {

 public:
   ModuleExit(const int moduleID, const int rank);

 private:
};

class Compute: public Message {

 public:
   Compute(const int moduleID, const int rank);

 private:
};

class CreateInputPort: public Message {

 public:
   CreateInputPort(const int moduleID, const int rank,
                   const std::string &name);

   const char *getName() const;

 private:
   char name[32];
};

class CreateOutputPort: public Message {

 public:
   CreateOutputPort(const int moduleID, const int rank,
                    const std::string &name);

   const char *getName() const;

 private:
   char name[32];
};

class AddObject: public Message {

 public:
   AddObject(const int moduleID, const int rank,
             const std::string &portName, const std::string &objectName);

   const char *getPortName() const;
   const char *getObjectName() const;

 private:
   char portName[32];
   char objectName[32];
};

class Connect: public Message {

 public:
   Connect(const int moduleIDA, const int moduleIDB,
           const std::string &portA, const std::string &portB);

   const char *getPortAName() const;
   const char *getPortBName() const;

   int getModuleA() const;
   int getModuleB() const;

 private:
   char portAName[32];
   char portBName[32];

   int moduleA;
   int moduleB;
};

} // namespace message
} // namespace vistle

#endif
