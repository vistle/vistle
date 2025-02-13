#ifndef VISTLE_CORE_MESSAGEROUTER_H
#define VISTLE_CORE_MESSAGEROUTER_H

#include "messages.h"
#include "export.h"

namespace vistle {

class Communicator;
class Parameter;
class Port;

namespace message {

enum RoutingFlags {

    Track = 0x000001,
    NodeLocal = 0x000002,
    ClusterLocal = 0x000004,

    DestMasterHub = 0x000008,
    DestSlaveHub = 0x000010,
    DestLocalHub = 0x000020,
    DestHub = DestMasterHub | DestSlaveHub,
    DestUi = 0x000040,
    DestModules = 0x000080,
    DestMasterManager = 0x000100,
    DestSlaveManager = 0x000200,
    DestLocalManager = 0x000400,
    DestManager = DestSlaveManager | DestMasterManager,

    Special = 0x000800,
    RequiresSubscription = 0x001000,

    //Broadcast            = DestHub|DestUi|DestManager,
    Broadcast = 0x100000,
    BroadcastModule = Broadcast | DestModules,

    HandleOnNode = 0x002000,
    HandleOnRank0 = 0x004000,
    HandleOnHub = 0x008000,
    HandleOnMaster = 0x010000,
    HandleOnDest = 0x020000,

    QueueIfUnhandled = 0x040000,
    TriggerQueue = 0x080000,
    CleanQueue = 0x100000,

    OnlyRank0 = 0x200000,
};

class V_COREEXPORT Router {
    friend class Message;

public:
    static Router &the();
    static void init(Identify::Identity identity, int hubId);
    static bool toRank0(const Message &msg);

    bool toUi(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN);
    bool toMasterHub(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN,
                     int senderHub = Id::Invalid);
    bool toSlaveHub(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN, int senderHub = Id::Invalid);
    bool toManager(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN, int senderHub = Id::Invalid);
    bool toModule(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN);
    bool toTracker(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN);
    bool toHandler(const Message &msg, Identify::Identity senderType = Identify::UNKNOWN);

private:
    static unsigned rt[NumMessageTypes];
    Router();
    Identify::Identity m_identity;
    int m_hubId;

    static void initRoutingTable();
};

} // namespace message
} // namespace vistle

#endif
