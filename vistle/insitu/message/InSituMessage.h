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
#include <core/messagequeue.h>
#include <core/tcpmessage.h>
#include <core/archives_config.h>
#include <core/archives.h>

#include <util/vecstreambuf.h>

namespace insitu{
namespace message {

enum class V_INSITUMESSAGEEXPORT InSituMessageType {
    Invalid
    , ShmInit
    , AddObject
    , AddPorts
    , AddCommands
    , Ready
    , ExecuteCommand
    , GoOn
    , ConstGrids
    , NthTimestep
    , ConnectionClosed
    , CycleFinished
    , SyncShmID

};


struct V_INSITUMESSAGEEXPORT InSituMessageBase {
    InSituMessageBase(InSituMessageType t) :m_type(t) {};
    InSituMessageType type() const;
protected:

    InSituMessageType m_type;

};

#define DECLARE_ENGINE_MESSAGE_WITH_PARAM(messageType, payloadName, payloadType)\
struct V_INSITUMESSAGEEXPORT messageType : public InSituMessageBase\
{\
    friend class InSituTcpMessage;\
    messageType(const payloadType& payloadName):InSituMessageBase(type), m_##payloadName(payloadName){}\
    static const InSituMessageType type = InSituMessageType::messageType;\
    payloadType m_##payloadName; \
    ARCHIVE_ACCESS\
    template<class Archive>\
    void serialize(Archive& ar) {\
       ar& m_##payloadName;\
    }\
private:\
    messageType():InSituMessageBase(type){}\
};\


#define DECLARE_ENGINE_MESSAGE(messageType)\
struct V_INSITUMESSAGEEXPORT messageType : public InSituMessageBase {\
    static const InSituMessageType type = InSituMessageType::messageType;\
    messageType() :InSituMessageBase(type) {}\
    ARCHIVE_ACCESS\
    template<class Archive>\
    void serialize(Archive& ar) {}\
};\

DECLARE_ENGINE_MESSAGE(Invalid)
DECLARE_ENGINE_MESSAGE(GoOn)
DECLARE_ENGINE_MESSAGE(ConnectionClosed)

DECLARE_ENGINE_MESSAGE_WITH_PARAM(ShmInit, shmName, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(AddObject, name, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(AddPorts, portList, std::vector<std::string>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(AddCommands, commandList, std::vector<std::string>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(Ready, state, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ExecuteCommand, command, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ConstGrids, state, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(NthTimestep, frequency, size_t)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(SyncShmID, ids, std::vector<std::size_t>)


struct V_INSITUMESSAGEEXPORT InSituMessage : public vistle::message::MessageBase<InSituMessage, vistle::message::INSITU> {
    InSituMessage(InSituMessageType t) :m_ismType(t) {}
    InSituMessageType ismType() const {
        return m_ismType;
    }
private:
    InSituMessageType m_ismType;
};
static_assert(sizeof(InSituMessage) <= vistle::message::Message::MESSAGE_SIZE, "message too large");




//after being initialized, sends and receives messages in a blocking manner. 
//When the connection is closed returns EngineMEssageType::ConnectionClosed and becomes uninitialized.
//while uninitialized calls to send and received are ignored.
//Received Messages are broadcasted to all ranks so make sure they all call receive together.
class V_INSITUMESSAGEEXPORT InSituTcpMessage {
public:

    InSituMessageType type() const;

    static void initialize(std::shared_ptr< boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm);
    static bool isInitialized();
    template<typename SomeMessage>
    static bool send(const SomeMessage& msg) {
        if (!m_initialized) {
            std::cerr << "Engine message uninitialized: can not send message!" << std::endl;
            return false;
        }
        if (msg.type == InSituMessageType::Invalid) {
            std::cerr << "Engine message : can not send invalid message!" << std::endl;
            return false;
        }
        bool error = false;
        if (m_comm.rank() == 0) {
            boost::system::error_code err;
            InSituMessage ism(msg.type);
            vistle::vecostreambuf<char> buf;
            vistle::oarchive ar(buf);
            ar& msg;
            vistle::buffer vec = buf.get_vector();

            ism.setPayloadSize(vec.size());
            vistle::message::send(*m_socket, ism, err, &vec);
            if (err) {
                error = true;
            }
        }
        return error;
    }

    static InSituTcpMessage recv();

    template<typename SomeMessage>
    SomeMessage& unpackOrCast() {


        assert(SomeMessage::type != type());
        if (!m_msg) {
            vistle::vecistreambuf<char> buf(m_payload);
            m_msg.reset(new SomeMessage{});
            try {
                vistle::iarchive ar(buf);
                ar&* static_cast<SomeMessage*>(m_msg.get());
            } catch (yas::io_exception & ex) {
                std::cerr << "ERROR: failed to get engine message payload from " << m_payload.size() << " bytes: " << ex.what() << std::endl;
            }
        }
        return *static_cast<SomeMessage*>(m_msg.get());

    }



private:
    InSituTcpMessage(InSituMessageType type, vistle::buffer&& payload);
    InSituTcpMessage();
    InSituMessageType m_type;
    vistle::buffer m_payload;
    std::unique_ptr<InSituMessageBase> m_msg;

    static bool m_initialized;
    static boost::mpi::communicator m_comm;
    static std::shared_ptr< boost::asio::ip::tcp::socket> m_socket;




};

class V_INSITUMESSAGEEXPORT InSituShmMessage {
public:
    InSituMessageType type() const;
    enum class Mode {
        Create
        , Attach
    };

    static void initialize(int moduleID, int rank, Mode mode);
    static bool isInitialized();
    template<typename SomeMessage>
    static bool send(const SomeMessage& msg) {
        if (!m_initialized) {
            std::cerr << "Engine message uninitialized: can not send message!" << std::endl;
            return false;
        }
        if (msg.type == InSituMessageType::Invalid) {
            std::cerr << "Engine message : can not send invalid message!" << std::endl;
            return false;
        }
        bool error = false;
        InSituMessage ism(msg.type);
        vistle::vecostreambuf<char> buf;
        vistle::oarchive ar(buf);
        ar& msg;
        vistle::buffer vec = buf.get_vector();
        vistle::MessagePayload pl;
        pl.construct(buf.get_vector().size());
        std::copy(buf.get_vector().begin(), buf.get_vector().end(), pl->data());
        pl.ref();
        ism.setPayloadName(pl.name());
        ism.setPayloadSize(vec.size());
#ifdef MODULE_THREAD
        ism.setSenderId(m_moduleID);
        ism.setRank(m_rank);
#endif
        m_sendMessageQueue->send(ism);
        return error;
    }

    static InSituShmMessage recv(bool block = true);

    template<typename SomeMessage>
    SomeMessage& unpackOrCast() {
        assert(SomeMessage::type != type());
        if (!m_msg) {
            assert(m_payloadSize > 0);
            vistle::MessagePayload pl;
            pl = vistle::Shm::the().getArrayFromName<char>(m_payload);
            pl.unref();
            assert(m_payloadSize == pl->size());
            vistle::vecistreambuf<char> buf(pl);
            m_msg.reset(new SomeMessage{});
            try {
                vistle::iarchive ar(buf);
                ar&* static_cast<SomeMessage*>(m_msg.get());
            } catch (yas::io_exception & ex) {
                std::cerr << "ERROR: failed to get insitu message payload from " << m_payloadSize << " bytes: " << ex.what() << std::endl;
            }
        }
        return *static_cast<SomeMessage*>(m_msg.get());

    }



private:
    InSituShmMessage(const InSituMessage& msg);
    InSituShmMessage();
    static std::unique_ptr<vistle::message::MessageQueue> m_sendMessageQueue;
    static std::unique_ptr<vistle::message::MessageQueue> m_receiveMessageQueue;
    static int m_rank;
    static int m_moduleID;

    InSituMessageType m_type;
    std::string m_payload;
    size_t m_payloadSize = 0;
    std::unique_ptr<InSituMessageBase> m_msg;

    static bool m_initialized;
};
} //message
} //insitu

#endif // !ENGINE_MESSAGE_H
