#ifndef VISTLE_MESSAGE_H
#define VISTLE_MESSAGE_H

#include <array>
#include <cassert>

#include <vistle/util/enum.h>
#include <vistle/util/buffer.h>
#include <vistle/util/exception.h>
#include "uuid.h"
#include "export.h"
#include "shmname.h"

#pragma pack(push)
#pragma pack(1)

namespace vistle {

namespace message {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(CompressionMode,
                                    (CompressionNone)(CompressionLz4)(CompressionZstd)(CompressionSnappy))

// clang-format off
DEFINE_ENUM_WITH_STRING_CONVERSIONS(
    Type,
    (INVALID) // keep 1st
    (ANY) //< for Trace: enables tracing of all message types -- keep 2nd
    (IDENTIFY)
    (CLOSECONNECTION)
    (ADDHUB)
    (REMOVEHUB)
    (SETID)
    (TRACE)
    (SPAWN)
    (SPAWNPREPARED)
    (KILL)
    (DEBUG)
    (QUIT)
    (STARTED)
    (MODULEEXIT)
    (BUSY)
    (IDLE)
    (SCREENSHOT)
    (EXECUTE)
    (EXECUTIONPROGRESS)
    (EXECUTIONDONE)
    (CANCELEXECUTE)
    (ADDOBJECT)
    (ADDOBJECTCOMPLETED)
    (DATATRANSFERSTATE)
    (ADDPORT)
    (REMOVEPORT)
    (CONNECT)
    (DISCONNECT)
    (ADDPARAMETER)
    (REMOVEPARAMETER)
    (SETPARAMETER)
    (SETPARAMETERCHOICES)
    (PING)
    (PONG)
    (BARRIER)
    (BARRIERREACHED)
    (SENDTEXT)
    (ITEMINFO)
    (UPDATESTATUS)
    (OBJECTRECEIVEPOLICY)
    (SCHEDULINGPOLICY)
    (REDUCEPOLICY)
    (MODULEAVAILABLE)
    (CREATEMODULECOMPOUND)
    (LOCKUI)
    (REPLAYFINISHED)
    (REQUESTTUNNEL)
    (REQUESTOBJECT)
    (SENDOBJECT)
    (REMOTERENDERING)
    (FILEQUERY)
    (FILEQUERYRESULT)
    (COVER)
    (INSITU)
    (NumMessageTypes) // keep last
)
V_ENUM_OUTPUT_OP(Type, ::vistle::message)
// clang-format on

struct V_COREEXPORT Id {
    enum Reserved {
        ModuleBase = 1, //< >= ModuleBase: modules
        Invalid = 0,
        Vistle = -1, //< session parameters
        Broadcast = -2, //< master is broadcasting to all modules/hubs
        ForBroadcast = -3, //< to master for broadcasting
        NextHop = -4,
        UI = -5,
        LocalManager = -6,
        LocalHub = -7,
        MasterHub = -8, //< < MasterHub: slave hubs
    };

    static bool isHub(int id);
    static bool isModule(int id);
    static std::string toString(Reserved id);
    static std::string toString(int id);
};

class V_COREEXPORT DefaultSender {
public:
    DefaultSender();
    static void init(int id, int rank);
    static const DefaultSender &instance();
    static int id();
    static int rank();

private:
    int m_id;
    int m_rank;
    static DefaultSender s_instance;
};

class V_COREEXPORT MessageFactory {
public:
    MessageFactory(int id = Id::Invalid, int rank = -1);

    int id() const;
    void setId(int id);
    int rank() const;
    void setRank(int rank);

    template<class M, class... P>
    M message(P &&...p) const
    {
        M m(std::forward<P>(p)...);
        m.setSenderId(m_id);
        m.setRank(m_rank);
        return m;
    }

private:
    int m_id = Id::Invalid;
    int m_rank = -1;
};

const int ModuleNameLength = 50;

typedef std::array<char, ModuleNameLength> module_name_t;
typedef std::array<char, 32> port_name_t;
typedef std::array<char, 32> param_name_t;
typedef std::array<char, 256> param_value_t;
typedef std::array<char, 50> param_choice_t;
typedef std::array<char, 300> shmsegname_t;
typedef std::array<char, 350> description_t;
typedef std::array<char, 200> address_t;
typedef std::array<char, 500> path_t;

typedef boost::uuids::uuid uuid_t;


class V_COREEXPORT Message {
    // this is POD

public:
    static const size_t MESSAGE_SIZE = 1024; // fixed message size is imposed by boost::interprocess::message_queue

    Message(const Type type, const unsigned int size);
    // Message (or its subclasses) may not require destructors

    //! processing flags for messages of a type, composed of RoutingFlags
    unsigned long typeFlags() const;

    //! set message uuid
    void setUuid(const uuid_t &uuid);
    //! message uuid
    const uuid_t &uuid() const;
    //! set uuid of message which this message reacts to
    void setReferrer(const uuid_t &ref);
    //! message this message refers to
    const uuid_t &referrer() const;
    //! message type
    Type type() const;
    //! sender ID
    int senderId() const;
    //! set sender ID
    void setSenderId(int id);
    //! sender rank
    int rank() const;
    //! set sender rank
    void setRank(int rank);
    //! UI id, if sent from a UI
    int uiId() const;
    //! message size
    size_t size() const;
    //! message has to be broadcast to all ranks?
    bool isForBroadcast() const;
    //! mark message for broadcast to all ranks on destination
    void setForBroadcast(bool enable = true);
    //! was message broadcast to all ranks?
    bool wasBroadcast() const;
    //! mark message as broadcast to all ranks
    void setWasBroadcast(bool enable = true);
    //! message is not a request for action, just a notification that an action has been taken
    bool isNotification() const;
    //! mark message as notification
    void setNotify(bool enable);

    //! id of message destination
    int destId() const;
    //! set id of message destination
    void setDestId(int id);

    //! rank of message destination
    int destRank() const;
    //! set rank of destination
    void setDestRank(int r);
    //! id of destination UI
    int destUiId() const;
    //! set id of destination UI
    void setDestUiId(int id);
    //! number of additional data bytes following message
    size_t payloadSize() const;
    //! set payload size
    void setPayloadSize(size_t size);
    //! retrieve name of payload in shared memory
    std::string payloadName() const;
    //! set name of payload in shared memory
    void setPayloadName(const shm_name_t &name);
    //! compression method for payload
    CompressionMode payloadCompression() const;
    //! set compression method for payload
    void setPayloadCompression(CompressionMode mode);
    //! number of uncompressed payload bytes
    size_t payloadRawSize() const;
    //! set number of uncompressed payload bytes
    void setPayloadRawSize(size_t size);
    //! set priority of this message in shm message queues
    void setPriority(unsigned int prio);
    //! get priority of this message in shm message queues
    unsigned int priority() const;
    // to be used with setPriority, ...
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(MessagePriority, (Normal)(ImmediateParameter))

    template<class SomeMessage>
    SomeMessage &as()
    {
        SomeMessage *m = static_cast<SomeMessage *>(this);
        assert(m->type() == SomeMessage::s_type);
        return *m;
    }
    template<class SomeMessage>
    SomeMessage const &as() const
    {
        const SomeMessage *m = static_cast<const SomeMessage *>(this);
        assert(m->type() == SomeMessage::s_type);
        return *m;
    }

private:
    //! message type
    Type m_type;
    //! message size
    unsigned int m_size;
    //! sender ID
    int m_senderId;
    //! sender rank
    int m_rank;
    //! destination ID
    int m_destId;
    //! destination rank - -1: dependent on message, most often current rank
    int m_destRank;
    //! message uuid
    uuid_t m_uuid;
    //! uuid of triggering message
    uuid_t m_referrer;

protected:
    //! payload size
    uint64_t m_payloadSize;
    //! raw (uncompressed) payload size
    uint64_t m_payloadRawSize;
    //! payload compression method
    int m_payloadCompression;
    //! name of payload in shared memory
    shm_name_t m_payloadName;
    //! priority in shmem message queues
    unsigned int m_priority;
    //! broadcast to all ranks?
    bool m_forBroadcast, m_wasBroadcast;
    //! message is not a request for action
    bool m_notification;
    //! pad message to multiple of 8 bytes
    char m_pad[5] = {};
};
// ensure alignment
static_assert(sizeof(Message) % sizeof(double) == 0, "not padded to ensure double alignment");

class V_COREEXPORT Buffer: public Message {
public:
    Buffer(): Message(ANY, Message::MESSAGE_SIZE) { memset(payload.data(), 0, payload.size()); }
    Buffer(const Message &message): Message(message)
    {
        memset(payload.data(), 0, payload.size());
        memcpy(payload.data(), (char *)&message + sizeof(Message), message.size() - sizeof(Message));
    }
    const Buffer &operator=(const Buffer &rhs)
    {
        *static_cast<Message *>(this) = rhs;
        memcpy(payload.data(), rhs.payload.data(), payload.size());
        return *this;
    }

    template<class SomeMessage>
    SomeMessage &as()
    {
        SomeMessage *m = static_cast<SomeMessage *>(static_cast<Message *>(this));
        assert(m->type() == SomeMessage::s_type);
        return *m;
    }
    template<class SomeMessage>
    SomeMessage const &as() const
    {
        const SomeMessage *m = static_cast<const SomeMessage *>(static_cast<const Message *>(this));
        assert(m->type() == SomeMessage::s_type);
        return *m;
    }

    size_t bufferSize() const { return Message::MESSAGE_SIZE; }
    size_t size() const { return Message::size(); }
    char *data() { return static_cast<char *>(static_cast<void *>(this)); }
    const char *data() const { return static_cast<const char *>(static_cast<const void *>(this)); }

private:
    std::array<char, Message::MESSAGE_SIZE - sizeof(Message)> payload;
};
static_assert(sizeof(Buffer) == Message::MESSAGE_SIZE, "message too large");

template<class MessageClass, Type MessageType>
class MessageBase: public Message {
public:
    static const Type s_type = MessageType;

protected:
    MessageBase(): Message(MessageType, sizeof(MessageClass))
    {
        static_assert(sizeof(MessageClass) <= Message::MESSAGE_SIZE, "message too large");
    }
};

V_COREEXPORT buffer compressPayload(vistle::message::CompressionMode &mode, const char *raw, size_t size,
                                    int speed = -1 /* algorithm default */);
V_COREEXPORT buffer compressPayload(vistle::message::CompressionMode &mode, const buffer &raw,
                                    int speed = -1 /* algorithm default */);
V_COREEXPORT buffer compressPayload(vistle::message::CompressionMode mode, Message &msg, buffer &raw,
                                    int speed = -1 /* algorithm default */);
V_COREEXPORT buffer decompressPayload(CompressionMode mode, size_t size, size_t rawsize, const char *compressed);
V_COREEXPORT buffer decompressPayload(vistle::message::CompressionMode mode, size_t size, size_t rawsize,
                                      buffer &compressed);
V_COREEXPORT buffer decompressPayload(const Message &msg, buffer &compressed);

V_COREEXPORT std::ostream &operator<<(std::ostream &s, const Message &msg);

class V_COREEXPORT codec_error: public vistle::exception {
public:
    codec_error(const std::string &what = "unsupported codec");
};

} // namespace message
} // namespace vistle

#pragma pack(pop)
#endif
