#include "message.h"

namespace vistle {
namespace message {

Message::Message(unsigned int t, unsigned int s): size(s), type(t) {
         
}
   
unsigned int Message::getType() {
      
   return type;
}

Debug::Debug(char c): Message(Message::DEBUG, sizeof(Debug)),
                      character(c) {
}

char Debug::getCharacter() {

   return character;
}
   
Spawn::Spawn(int m): Message(Message::SPAWN, sizeof(Spawn)),
                     moduleID(m) {

}

int Spawn::getModuleID() {

   return moduleID;
}

Quit::Quit(): Message(Message::QUIT, sizeof(Quit)) {
}
   
} // namespace message
} // namespace vistle
