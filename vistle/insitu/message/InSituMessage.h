#ifndef INSITU_MESSAGE_H
#define INSITU_MESSAGE_H

#include "export.h"
#include "moduleInfo.h"
#include "sharedOption.h"

#include <array>
#include <string>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/mpi.hpp>
#include <vistle/core/archives.h>
#include <vistle/core/archives_config.h>
#include <vistle/core/message.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/messages.h>
#include <vistle/core/shm.h>
#include <vistle/core/tcpmessage.h>

#include <vistle/util/vecstreambuf.h>

#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace vistle {
namespace insitu {

namespace message {

enum class InSituMessageType {
    Invalid,
    AddObject,
    ConnectionClosed,
    ExecuteCommand,
    GoOn,
    LibSimIntOption,
    Ready,
    SenseiIntOption,
    SetCommands,
    SetCustomCommands,
    SetPorts, // detected ports from sim to module <--> connected ports from module
    // to Engine
    ShmInfo
#ifdef MODULE_THREAD
    ,
    ModuleID
#endif
};

class Message;
struct V_INSITUMESSAGEEXPORT InSituMessageBase {
    InSituMessageBase(InSituMessageType t): m_type(t){};
    InSituMessageType type() const;
    virtual ~InSituMessageBase() = default;

protected:
    InSituMessageType m_type;
};

template<InSituMessageType Type, typename T>
struct InSituValueMessage: InSituMessageBase {
    typedef T value_type;
    friend class insitu::message::Message;
    InSituValueMessage(const T &payload): InSituMessageBase(Type), value(payload) {}
    static const InSituMessageType type = Type;
    T value{};
    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &value;
    }

private:
    InSituValueMessage(): InSituMessageBase(Type) {}
};

template<InSituMessageType Type>
struct InSituPureMessage: InSituMessageBase {
    InSituPureMessage(): InSituMessageBase(Type) {}
    static const InSituMessageType type = Type;
    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {}
};

#define COMMA ,

#define DEFINE_IN_SITU_MESSAGE(messageType, payloadType) \
    typedef InSituValueMessage<InSituMessageType::messageType, payloadType> messageType;
#define DEFINE_IN_SITU_MESSAGE_NO_PARAM(messageType) \
    typedef InSituPureMessage<InSituMessageType::messageType> messageType;

DEFINE_IN_SITU_MESSAGE_NO_PARAM(Invalid)
DEFINE_IN_SITU_MESSAGE_NO_PARAM(GoOn)

DEFINE_IN_SITU_MESSAGE(ConnectionClosed, bool) // true -> disconnected on purpose
DEFINE_IN_SITU_MESSAGE(ShmInfo, ModuleInfo::ShmInfo)
DEFINE_IN_SITU_MESSAGE(AddObject, std::string)
DEFINE_IN_SITU_MESSAGE(SetPorts, std::vector<std::vector<std::string>>)
DEFINE_IN_SITU_MESSAGE(SetCommands, std::vector<std::string>)
DEFINE_IN_SITU_MESSAGE(SetCustomCommands, std::vector<std::string>)
DEFINE_IN_SITU_MESSAGE(Ready, bool)
DEFINE_IN_SITU_MESSAGE(
    ExecuteCommand,
    std::pair<std::string COMMA std::string>) //command name + empty string for generic or + value for custom

#ifdef MODULE_THREAD
DEFINE_IN_SITU_MESSAGE(ModuleID, int)
#endif

struct V_INSITUMESSAGEEXPORT InSituMessage
: public vistle::message::MessageBase<InSituMessage, vistle::message::INSITU> {
    InSituMessage(InSituMessageType t): m_ismType(t) {}
    InSituMessageType ismType() const { return m_ismType; }

private:
    InSituMessageType m_ismType;
};
static_assert(sizeof(InSituMessage) <= vistle::message::Message::MESSAGE_SIZE, "message too large");

class V_INSITUMESSAGEEXPORT Message {
public:
    InSituMessageType type() const;

    template<typename SomeMessage>
    SomeMessage &unpackOrCast() const
    {
        assert(SomeMessage::type == type());
        if (!m_msg) {
            vistle::vecistreambuf<vistle::buffer> buf(m_payload);
            m_msg.reset(new SomeMessage{});
            try {
                vistle::iarchive ar(buf);
                ar &*static_cast<SomeMessage *>(m_msg.get());
#ifdef USE_YAS
            } catch (yas::io_exception &ex) {
                std::cerr << "ERROR: failed to get InSituTcp::Message of type " << static_cast<int>(type())
                          << " with payload size " << m_payload.size() << " bytes: " << ex.what() << std::endl;
#endif
            } catch (...) {
                throw;
            }
        }
        return *static_cast<SomeMessage *>(m_msg.get());
    }
    static Message errorMessage();
    Message(InSituMessageType type, vistle::buffer &&payload);

private:
    Message();
    InSituMessageType m_type;
    vistle::buffer m_payload;
    mutable std::unique_ptr<InSituMessageBase> m_msg;
};

} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !ENGINE_MESSAGE_H
