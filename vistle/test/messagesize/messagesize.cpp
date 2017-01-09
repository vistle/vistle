#include <iostream>
#include <core/message.h>
#include <core/messages.h>

using namespace vistle;
using namespace vistle::message;

#define M(t, T) case message::t: { \
   const T m = msg.as<T>(); \
   std::cerr << i << " size: " << sizeof(m) << ", type: " << type << std::endl; \
   break; \
}

int main() {

   Buffer msg;
   for (int i=message::INVALID; i<message::NumMessageTypes; ++i) {

      const message::Type type = static_cast<message::Type>(i);
      switch(type) {
         case message::INVALID: {
         case message::ANY:
         case message::NumMessageTypes:
            break;
         }

         M(IDENTIFY, Identify)
         M(ADDHUB, AddHub)
         M(SETID, SetId)
         M(TRACE, Trace)
         M(SPAWN, Spawn)
         M(SPAWNPREPARED, SpawnPrepared)
         M(KILL, Kill)
         M(QUIT, Quit)
         M(STARTED, Started)
         M(MODULEEXIT, ModuleExit)
         M(BUSY, Busy)
         M(IDLE, Idle)
         M(EXECUTIONPROGRESS, Idle)
         
         M(EXECUTE, Execute)
         M(ADDOBJECT, AddObject)
         M(ADDOBJECTCOMPLETED, AddObjectCompleted)
         M(OBJECTRECEIVED, ObjectReceived)
         M(ADDPORT, AddPort)
         M(REMOVEPORT, AddPort)
         M(CONNECT, Connect)
         M(DISCONNECT, Disconnect)
         M(ADDPARAMETER, AddParameter)
         M(REMOVEPARAMETER, AddParameter)
         M(SETPARAMETER, SetParameter)
         M(SETPARAMETERCHOICES, SetParameterChoices)
         M(PING, Ping)
         M(PONG, Pong)
         M(BARRIER, Barrier)
         M(BARRIERREACHED, BarrierReached)
         M(SENDTEXT, SendText)
         M(OBJECTRECEIVEPOLICY, ObjectReceivePolicy)
         M(SCHEDULINGPOLICY, SchedulingPolicy)
         M(REDUCEPOLICY, ReducePolicy)
         M(MODULEAVAILABLE, ModuleAvailable)
         M(LOCKUI, LockUi)
         M(REPLAYFINISHED, ReplayFinished)
         M(REQUESTTUNNEL, RequestTunnel)
         M(REQUESTOBJECT, RequestObject)
         M(SENDOBJECT, SendObject)

         default: {
            std::cerr << i << ": " << type << std::endl;
         }
      }
   }
   return 0;
}
