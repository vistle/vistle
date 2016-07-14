#include "message.h"
#include "messagerouter.h"
#include "shm.h"
#include "parameter.h"
#include "port.h"


namespace vistle {
namespace message {


unsigned Router::rt[Message::NumMessageTypes];

void Router::initRoutingTable() {

   typedef Message M;
   memset(&rt, '\0', sizeof(rt));

   rt[M::INVALID]               = 0;
   rt[M::IDENTIFY]              = Special;
   rt[M::SETID]                 = Special;
   rt[M::ADDHUB]                = Broadcast|Track|DestUi;
   rt[M::REMOVESLAVE]           = Broadcast|Track|DestUi;
   rt[M::REPLAYFINISHED]        = Special;
   rt[M::TRACE]                 = Broadcast|Track;
   rt[M::SPAWN]                 = Track|HandleOnMaster;
   rt[M::SPAWNPREPARED]         = DestLocalHub|HandleOnHub;
   rt[M::STARTED]               = Track|DestUi|DestManager|DestModules|OnlyRank0;
   rt[M::MODULEEXIT]            = Track|DestUi|DestManager|DestModules|OnlyRank0;
   rt[M::KILL]                  = DestModules|HandleOnDest;
   rt[M::QUIT]                  = Broadcast|HandleOnMaster|HandleOnHub|HandleOnNode;
   rt[M::EXECUTE]               = Special|HandleOnMaster;
   rt[M::MODULEAVAILABLE]       = Track|DestHub|DestUi|HandleOnHub;
   rt[M::ADDPORT]               = Track|DestUi|DestManager|DestModules|TriggerQueue|OnlyRank0;
   rt[M::ADDPARAMETER]          = Track|DestUi|DestManager|DestModules|TriggerQueue|OnlyRank0;
   rt[M::SETPARAMETERCHOICES]   = Track|DestUi|DestModules|OnlyRank0;
   rt[M::CONNECT]               = Track|Broadcast|QueueIfUnhandled|DestManager|DestModules|OnlyRank0;
   rt[M::DISCONNECT]            = Track|Broadcast|QueueIfUnhandled|DestManager|DestModules|OnlyRank0;
   rt[M::SETPARAMETER]          = Track|QueueIfUnhandled|DestManager|DestUi|DestModules|OnlyRank0;
   rt[M::PING]                  = DestManager|DestModules|HandleOnDest;
   rt[M::PONG]                  = DestUi|HandleOnDest;
   rt[M::BUSY]                  = Special;
   rt[M::IDLE]                  = Special;
   rt[M::LOCKUI]                = DestUi;
   rt[M::SENDTEXT]              = DestUi|DestMasterHub;

   rt[M::OBJECTRECEIVEPOLICY]   = DestLocalManager|Track;
   rt[M::SCHEDULINGPOLICY]      = DestLocalManager|Track;
   rt[M::REDUCEPOLICY]          = DestLocalManager|Track;
   rt[M::EXECUTIONPROGRESS]     = DestManager|HandleOnRank0;

   rt[M::ADDOBJECT]             = DestManager|HandleOnNode;
   rt[M::ADDOBJECTCOMPLETED]    = DestManager|HandleOnNode;

   rt[M::BARRIER]               = HandleOnDest;
   rt[M::BARRIERREACHED]        = HandleOnDest;
   rt[M::OBJECTRECEIVED]        = HandleOnRank0;

   rt[M::REQUESTTUNNEL]         = HandleOnNode|HandleOnHub;

   rt[M::REQUESTOBJECT]         = Special;
   rt[M::SENDOBJECT]            = Special;

   for (int i=M::ANY+1; i<M::NumMessageTypes; ++i) {
      if (rt[i] == 0) {
         std::cerr << "message routing table not initialized for " << (Message::Type)i << std::endl;
      }
      vassert(rt[i] != 0);
   }
}

Router &Router::the() {

   static Router router;
   return router;
}

Router::Router() {

   m_identity = Identify::UNKNOWN;
   m_hubId = Id::Invalid;
   initRoutingTable();
}

void Router::init(Identify::Identity identity, int hubId) {

   the().m_identity = identity;
   the().m_hubId = hubId;
}

bool Router::toUi(const Message &msg, Identify::Identity senderType) {

   if (msg.destId() == Id::ForBroadcast)
      return false;
   if (msg.destId() >= Id::ModuleBase)
      return false;
   if (msg.destId() == Id::Broadcast)
      return true;
   if (msg.destId() == Id::UI)
      return true;
   const int t = msg.type();
   if (rt[t] & DestUi)
      return true;
   if (rt[t] & Broadcast)
      return true;

   return false;
}

bool Router::toMasterHub(const Message &msg, Identify::Identity senderType, int senderHub) {

   if (m_identity != Identify::SLAVEHUB)
      return false;
   if (senderType == Identify::HUB)
      return false;

   if (msg.destId() == Id::ForBroadcast)
      return true;
   if (msg.destId() == Id::Broadcast)
      return true;

   const int t = msg.type();
   if (rt[t] & DestMasterHub) {
      if (senderType != Identify::HUB)
         if (m_identity != Identify::HUB)
            return true;
   }

   if (rt[t] & DestSlaveHub) {
      return true;
   }

   if (rt[t] & Broadcast) {
      if (msg.senderId() == m_hubId)
         return true;
      std::cerr << "toMasterHub: sender id: " << msg.senderId() << ", hub: " << senderHub << std::endl;
      if (senderHub == m_hubId)
         return true;
   }

   return false;
}

bool Router::toSlaveHub(const Message &msg, Identify::Identity senderType, int senderHub) {

   if (msg.destId() == Id::ForBroadcast)
      return false;

   if (m_identity != Identify::HUB)
      return false;

   if (msg.destId() == Id::Broadcast) {
      return true;
   }

   const int t = msg.type();
   if (rt[t] & DestSlaveHub) {
      return true;
   }

#if 0
   if (m_identity == Identify::SLAVEHUB) {
      if (rt[t] & DestManager)
         return true;
   } else if (m_identity == Identify::HUB) {
      if (rt[t] & DestManager)
         return true;
   }
#endif

   if (rt[t] & Broadcast)
      return true;

   return false;
}

bool Router::toManager(const Message &msg, Identify::Identity senderType, int senderHub) {

   const int t = msg.type();
   if (msg.destId() == Id::ForBroadcast)
       return false;
   if (msg.destId() == Id::Broadcast)
       return true;
   if (Id::isHub(msg.destId())) {
      if (msg.destId() == m_hubId && rt[t] & DestManager) {
         return true;
      }
      return false;
   }
   if (senderType != Identify::MANAGER || senderHub!=m_hubId) {
      if (rt[t] & DestManager)
         return true;
      if  (rt[t] & DestModules)
         return true;
      if (rt[t] & Broadcast)
         return true;
   }

   return false;
}

bool Router::toModule(const Message &msg, Identify::Identity senderType) {

   if (msg.destId() == Id::ForBroadcast)
      return false;

   const int t = msg.type();
   if (rt[t] & DestModules)
      return true;
   if (rt[t] & Broadcast)
      return true;

   return false;
}

bool Router::toTracker(const Message &msg, Identify::Identity senderType) {

   if (msg.destId() == Id::ForBroadcast)
      return false;

   const int t = msg.type();
   if (rt[t] & Track) {
      if (m_identity == Identify::HUB) {
         return senderType == Identify::SLAVEHUB || senderType == Identify::MANAGER;
      }
      if (m_identity == Identify::SLAVEHUB) {
         return senderType == Identify::HUB || senderType == Identify::MANAGER;
      }
   }

   return false;
}

bool Router::toHandler(const Message &msg, Identify::Identity senderType) {

   const int t = msg.type();
   if (msg.destId() == Id::NextHop || msg.destId() == Id::Broadcast || msg.destId() == m_hubId) {
      return true;
   }
   if (m_identity == Identify::HUB) {
      if (rt[t] & HandleOnMaster) {
         return true;
      }
      if (rt[t] & DestMasterHub)
         return true;
   }
   if (m_identity == Identify::SLAVEHUB || m_identity == Identify::HUB) {
      if (rt[t] & HandleOnHub)
         return true;
      if (rt[t] & DestLocalHub)
         return true;
   }
   if (m_identity == Identify::SLAVEHUB) {
      if (rt[t] & DestSlaveHub)
         return true;
   }
   if (m_identity == Identify::MANAGER) {
      if (m_hubId == Id::MasterHub)
         return rt[t] & DestMasterManager;
      else
         return rt[t] & DestSlaveManager;
   }

   return false;
}

bool Router::toRank0(const Message &msg) {
   const int t = msg.type();
   return !(rt[t] & OnlyRank0);
}

} // namespace message
} // namespace vistle
