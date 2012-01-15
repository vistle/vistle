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
   enum {
      DEBUG            =  0,
      SPAWN            =  1,
      QUIT             =  2,
      NEWOBJECT        =  3,
      MODULEEXIT       =  4,
      COMPUTE          =  5,
      CREATEINPUTPORT  =  6,
      CREATEOUTPUTPORT =  7,
      ADDOBJECT        =  8,
   };

   Message(const unsigned int type, const unsigned int size);

   unsigned int getType() const;

 private:
   unsigned int size;
   const unsigned int type;
};

class Debug: public Message {

 public:
   Debug(const char c);

   char getCharacter() const;

 private:
   const char character;
};

class Spawn: public Message {

 public:
   Spawn(const int moduleID);

   int getModuleID() const;

 private:
   const int moduleID;
};

class Quit: public Message {

 public:
   Quit();

 private:
};

class NewObject: public Message {

 public:
   NewObject(const std::string &name);

   const char *getName() const;

 private:
   char name[32];
};

class ModuleExit: public Message {

 public:
   ModuleExit(const int moduleID, const int rank);

   int getModuleID() const;
   int getRank() const;

 private:
   const int moduleID;
   const int rank;
};

class Compute: public Message {

 public:
   Compute();

 private:
};

class CreateInputPort: public Message {

 public:
   CreateInputPort(const std::string &name);

   const char *getName() const;

 private:
   char name[32];
};

class CreateOutputPort: public Message {

 public:
   CreateOutputPort(const std::string &name);

   const char *getName() const;

 private:
   char name[32];
};

class AddObject: public Message {

 public:
   AddObject(const std::string &portName, const std::string &objectName);

   const char *getPortName() const;
   const char *getObjectName() const;

 private:
   char portName[32];
   char objectName[32];
};


} // namespace message
} // namespace vistle

#endif
