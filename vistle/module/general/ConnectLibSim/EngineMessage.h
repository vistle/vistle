#ifndef ENGINE_MESSAGE_H
#define ENGINE_MESSAGE_H

#include "VisItExports.h"

#include <string>
#include <array>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/mpi.hpp>
#include <core/message.h>
#include <core/messages.h>
#include <core/tcpmessage.h>
#include <core/archives_config.h>
#include <core/archives.h>

#include <util/vecstreambuf.h>

namespace insitu{


enum class V_VISITXPORT EngineMessageType {
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

};

struct V_VISITXPORT EngineMessageBase {
    EngineMessageBase(EngineMessageType t) :m_type(t) {};
    EngineMessageType type() const;
protected:

    EngineMessageType m_type;

};

#define DECLARE_ENGINE_MESSAGE_WITH_PARAM(messageType, payloadName, payloadType)\
struct V_VISITXPORT EM_##messageType : public EngineMessageBase\
{\
    friend class EngineMessage;\
    EM_##messageType(const payloadType& payloadName):EngineMessageBase(type), m_##payloadName(payloadName){}\
    static const EngineMessageType type = EngineMessageType::messageType;\
    payloadType m_##payloadName; \
    ARCHIVE_ACCESS\
    template<class Archive>\
    void serialize(Archive& ar) {\
       ar& m_##payloadName;\
    }\
private:\
    EM_##messageType():EngineMessageBase(type){}\
};\


#define DECLARE_ENGINE_MESSAGE(messageType)\
struct V_VISITXPORT EM_##messageType : public EngineMessageBase {\
    static const EngineMessageType type = EngineMessageType::messageType;\
    EM_##messageType() :EngineMessageBase(type) {}\
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
//use vector{true/false, Shm::the().objectID(), Shm::the().arrayID()} 
DECLARE_ENGINE_MESSAGE_WITH_PARAM(Ready, state, std::vector<size_t>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ExecuteCommand, command, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ConstGrids, state, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(NthTimestep, frequency, size_t)



struct V_VISITXPORT InSituMessage : public vistle::message::MessageBase<InSituMessage, vistle::message::INSITU> {
    InSituMessage(EngineMessageType t):emType(t){}
    EngineMessageType emType;
};
static_assert(sizeof(InSituMessage) <= vistle::message::Message::MESSAGE_SIZE, "message too large");




//after being initialized, sends and receives messages in a blocking manner. 
//When the connection is closed returns EngineMEssageType::ConnectionClosed and becomes uninitialized.
//while uninitialized calls to send and received are ignored.
//Received Messages are broadcasted to all ranks so make sure they all call receive together.
class V_VISITXPORT EngineMessage {
public:

    EngineMessageType type() const;

    static void initializeEngineMessage(std::shared_ptr< boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm);
    static bool isInitialized();
    template<typename SomeMessage>
    static bool sendEngineMessage(const SomeMessage& msg) {
        if (!m_initialized) {
            std::cerr << "Engine message uninitialized: can not send message!" << std::endl;
            return false;
        }
        if (msg.type == EngineMessageType::Invalid) {
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

    static EngineMessage recvEngineMessage();

    template<typename SomeMessage>
    SomeMessage& unpackOrCast() {


        assert(SomeMessage::type != type());
        if (!m_msg) {
            vistle::vecistreambuf<char> buf(m_payload);
            m_msg.reset(new SomeMessage{});
            try {
                vistle::iarchive ar(buf);
                ar& *static_cast<SomeMessage*>(m_msg.get());
            } catch (yas::io_exception & ex) {
                std::cerr << "ERROR: failed to get engine message payload from " << m_payload.size() << " bytes: " << ex.what() << std::endl;
            }
        }
        return *static_cast<SomeMessage*>(m_msg.get());

    }



private:
    EngineMessage(EngineMessageType type, vistle::buffer&& payload);
    EngineMessage();
    EngineMessageType m_type;
    vistle::buffer m_payload;
    std::unique_ptr<EngineMessageBase> m_msg;

    static bool m_initialized;
    static boost::mpi::communicator m_comm;
    static std::shared_ptr< boost::asio::ip::tcp::socket> m_socket;




};



}

#endif // !ENGINE_MESSAGE_H
