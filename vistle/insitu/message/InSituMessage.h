#ifndef INSITU_MESSAGE_H
#define INSITU_MESSAGE_H
#include "export.h"

#include <string>
#include <array>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/mpi.hpp>
#include <core/message.h>
#include <core/messages.h>
#include <core/messagepayload.h>
#include <core/tcpmessage.h>
#include <core/archives_config.h>
#include <core/archives.h>
#include <core/shm.h>

#include <util/vecstreambuf.h>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace vistle {
namespace insitu {

namespace message {

enum class V_INSITUMESSAGEEXPORT InSituMessageType {
    Invalid
    , ShmInit
    , AddObject
    , SetPorts //detected ports from sim to module <--> connected ports from module to Engine
    , SetCommands
    , Ready
    , ExecuteCommand
    , GoOn
    , ConstGrids
    , NthTimestep
    , ConnectionClosed
    , VTKVariables
    , CombineGrids
    , KeepTimesteps
#ifdef MODULE_THREAD
    , ModuleID
#endif
};

class Message;
struct V_INSITUMESSAGEEXPORT InSituMessageBase {
    InSituMessageBase(InSituMessageType t) :m_type(t) {};
    InSituMessageType type() const;
protected:

    InSituMessageType m_type;

};
#define COMMA ,

#define DECLARE_ENGINE_MESSAGE_WITH_PARAM(messageType,  payloadType)\
struct V_INSITUMESSAGEEXPORT messageType : public InSituMessageBase\
{\
    typedef payloadType value_type;\
    friend class insitu::message::Message; \
    messageType(const payloadType& payloadName):InSituMessageBase(type), value(payloadName){}\
     static const InSituMessageType type = InSituMessageType::messageType;\
    payloadType value{}; \
    ARCHIVE_ACCESS\
    template<class Archive>\
    void serialize(Archive& ar) {\
       ar& value;\
    }\
private:\
    messageType():InSituMessageBase(type){}\
};\



#define DECLARE_ENGINE_MESSAGE(messageType)\
struct V_INSITUMESSAGEEXPORT messageType : public InSituMessageBase {\
     const InSituMessageType type = InSituMessageType::messageType;\
    messageType() :InSituMessageBase(type) {}\
    ARCHIVE_ACCESS\
    template<class Archive>\
    void serialize(Archive& ar) {}\
};\

DECLARE_ENGINE_MESSAGE(Invalid)
DECLARE_ENGINE_MESSAGE(GoOn)

DECLARE_ENGINE_MESSAGE_WITH_PARAM(ConnectionClosed, bool) //true -> disconnected on purpose
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ShmInit, std::vector<std::string>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(AddObject, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(SetPorts, std::vector<std::vector<std::string>>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(SetCommands, std::vector<std::string>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(Ready, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ExecuteCommand, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ConstGrids, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(CombineGrids, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(NthTimestep, size_t)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(VTKVariables, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(KeepTimesteps, bool)
#ifdef MODULE_THREAD
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ModuleID, int)
#endif


struct V_INSITUMESSAGEEXPORT InSituMessage : public vistle::message::MessageBase<InSituMessage, vistle::message::INSITU> {
    InSituMessage(InSituMessageType t) :m_ismType(t) {}
    InSituMessageType ismType() const {
        return m_ismType;
    }
private:
    InSituMessageType m_ismType;
};
static_assert(sizeof(InSituMessage) <= vistle::message::Message::MESSAGE_SIZE, "message too large");

class V_INSITUMESSAGEEXPORT Message {

public:
    InSituMessageType type() const;


    template<typename SomeMessage>
    SomeMessage& unpackOrCast() {


        assert(SomeMessage::type != type());
        if (!m_msg) {
            vistle::vecistreambuf<vistle::buffer> buf(m_payload);
            m_msg.reset(new SomeMessage{});
            try {
                vistle::iarchive ar(buf);
                ar&* static_cast<SomeMessage*>(m_msg.get());
            }
            catch (yas::io_exception& ex) {
                std::cerr << "ERROR: failed to get InSituTcp::Message of type " << static_cast<int>(type()) << " with payload size " << m_payload.size() << " bytes: " << ex.what() << std::endl;
            }
        }
        return *static_cast<SomeMessage*>(m_msg.get());

    }
    static Message errorMessage();
    Message(InSituMessageType type, vistle::buffer&& payload);
private:

    Message();
    InSituMessageType m_type;
    vistle::buffer m_payload;
    std::unique_ptr<InSituMessageBase> m_msg;
};





} //message
} //insitu
} //vistle


#endif // !ENGINE_MESSAGE_H
