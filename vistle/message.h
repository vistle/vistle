#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

namespace vistle {

class Communicator;

namespace message {

class Message {

   friend class vistle::Communicator;

 public:      
   enum { DEBUG = 0, SPAWN = 1, QUIT = 2 };
   
   Message(unsigned int type, unsigned int size);
   
   unsigned int getType();

 private:
   unsigned int size;
   unsigned int type;
};

class Debug: public Message {

 public:
   Debug(char c);
   char getCharacter();

 private:
   char character;
};
 
class Spawn: public Message {

 public:
   Spawn(int moduleID);
   int getModuleID();

 private:
   int moduleID;
};

class Quit: public Message {

 public:
   Quit();
 private:
   char schnubb[32];
};

} // namespace message
} // namespace vistle

#endif
