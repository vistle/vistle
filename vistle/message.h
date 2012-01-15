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
      DEBUG      = 0,
      SPAWN      = 1,
      QUIT       = 2,
      NEWOBJECT  = 3,
      MODULEEXIT = 4
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
   char name[64];
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

} // namespace message
} // namespace vistle

#endif
