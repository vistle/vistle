#include "message.h"
#include "messagerouter.h"
#include <cassert>


namespace vistle {
namespace message {


unsigned Router::rt[NumMessageTypes];

void Router::initRoutingTable()
{
    memset(&rt, '\0', sizeof(rt));

    rt[INVALID] = 0;
    rt[IDENTIFY] = Special;
    rt[SETID] = Special;
    rt[ADDHUB] = Broadcast | Track | DestUi | TriggerQueue;
    rt[REMOVEHUB] = Broadcast | Track | DestUi | TriggerQueue;
    rt[REPLAYFINISHED] = Special;
    rt[TRACE] = Broadcast | DestHub | DestManager | DestModules | Track;
    rt[SPAWN] = Track | HandleOnMaster;
    rt[SPAWNPREPARED] = DestLocalHub | HandleOnHub;
    rt[STARTED] = Track | DestUi | DestManager | DestModules | OnlyRank0;
    rt[MODULEEXIT] = Track | DestUi | DestManager | DestModules | OnlyRank0 | CleanQueue;
    rt[KILL] = DestModules | HandleOnDest;
    rt[DEBUG] = Track | HandleOnHub;
    rt[QUIT] = Track | Special;
    rt[CLOSECONNECTION] = Track | Special;
    rt[EXECUTE] = Special | HandleOnMaster | OnlyRank0 | Track | DestUi;
    rt[EXECUTIONDONE] = Special | HandleOnMaster | OnlyRank0 | Track | TriggerQueue;
    rt[CANCELEXECUTE] = Special | HandleOnMaster | OnlyRank0;
    rt[MODULEAVAILABLE] = Track | DestHub | DestUi | HandleOnHub;
    rt[CREATEMODULECOMPOUND] = Track | DestHub | DestManager | HandleOnHub;
    rt[ADDPORT] = Track | DestUi | DestManager | DestModules | TriggerQueue | OnlyRank0 | HandleOnMaster;
    rt[REMOVEPORT] = Track | DestUi | DestManager | DestModules | OnlyRank0;
    rt[ADDPARAMETER] = Track | DestUi | DestManager | DestModules | TriggerQueue | OnlyRank0;
    rt[REMOVEPARAMETER] = Track | DestUi | DestManager | DestModules | OnlyRank0;
    rt[SETPARAMETERCHOICES] = Track | DestUi | DestModules | OnlyRank0;
    rt[CONNECT] = Special;
    rt[DISCONNECT] = Special;
    rt[SETPARAMETER] = Track | QueueIfUnhandled | DestManager | DestUi | DestModules | OnlyRank0;
    rt[PING] = DestManager | DestModules | HandleOnDest;
    rt[PONG] = DestUi | HandleOnDest;
    rt[BUSY] = DestUi | DestMasterHub;
    rt[IDLE] = DestUi | DestMasterHub;
    rt[LOCKUI] = DestUi;
    rt[SENDTEXT] = DestUi | DestMasterHub;
    rt[UPDATESTATUS] = Track | DestUi | DestMasterHub;

    rt[OBJECTRECEIVEPOLICY] = DestLocalManager | Track;
    rt[SCHEDULINGPOLICY] = DestLocalManager | Track;
    rt[REDUCEPOLICY] = DestLocalManager | Track;
    rt[EXECUTIONPROGRESS] = DestManager | HandleOnRank0;
    rt[DATATRANSFERSTATE] = Special;

    rt[ADDOBJECT] = DestManager | HandleOnNode;
    rt[ADDOBJECTCOMPLETED] = DestManager | HandleOnNode;

    rt[BARRIER] = HandleOnDest;
    rt[BARRIERREACHED] = HandleOnDest;

    rt[REQUESTTUNNEL] = HandleOnNode | HandleOnHub;

    rt[REQUESTOBJECT] = Special;
    rt[SENDOBJECT] = Special;
    rt[REMOTERENDERING] = Special;

    rt[FILEQUERY] = Special;
    rt[FILEQUERYRESULT] = Special;
    rt[COVER] = Special;
    rt[INSITU] = Special;

    for (int i = ANY + 1; i < NumMessageTypes; ++i) {
        if (rt[i] == 0) {
            std::cerr << "message routing table not initialized for " << (Type)i << std::endl;
        }
        assert(rt[i] != 0);
    }
}

Router &Router::the()
{
    static Router router;
    return router;
}

Router::Router()
{
    m_identity = Identify::UNKNOWN;
    m_hubId = Id::Invalid;
    initRoutingTable();
}

void Router::init(Identify::Identity identity, int hubId)
{
    the().m_identity = identity;
    the().m_hubId = hubId;
}

bool Router::toUi(const Message &msg, Identify::Identity senderType)
{
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

bool Router::toMasterHub(const Message &msg, Identify::Identity senderType, int senderHub)
{
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

bool Router::toSlaveHub(const Message &msg, Identify::Identity senderType, int senderHub)
{
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

bool Router::toManager(const Message &msg, Identify::Identity senderType, int senderHub)
{
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
    if (senderType != Identify::MANAGER || senderHub != m_hubId) {
        if (rt[t] & DestManager)
            return true;
        if (rt[t] & DestModules)
            return true;
        if (rt[t] & Broadcast)
            return true;
    }

    return false;
}

bool Router::toModule(const Message &msg, Identify::Identity senderType)
{
    if (msg.destId() == Id::ForBroadcast)
        return false;

    const int t = msg.type();
    if (rt[t] & DestModules)
        return true;
    if (rt[t] & Broadcast)
        return true;

    return false;
}

bool Router::toTracker(const Message &msg, Identify::Identity senderType)
{
    if (msg.destId() == Id::ForBroadcast)
        return false;

    const int t = msg.type();
    if (rt[t] & Track) {
        if (m_identity == Identify::HUB) {
            return senderType == Identify::SLAVEHUB || senderType == Identify::MANAGER || senderType == Identify::UI;
        }
        if (m_identity == Identify::SLAVEHUB) {
            return senderType == Identify::HUB || senderType == Identify::MANAGER || senderType == Identify::UI;
        }
    }

    return false;
}

bool Router::toHandler(const Message &msg, Identify::Identity senderType)
{
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

bool Router::toRank0(const Message &msg)
{
    const int t = msg.type();
    return !(rt[t] & OnlyRank0);
}

} // namespace message
} // namespace vistle
