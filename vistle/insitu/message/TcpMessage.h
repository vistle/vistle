#ifndef INSITU_TCP_MESSAGE_H
#define INSITU_TCP_MESSAGE_H
#include "InSituMessage.h"
#include "export.h"
namespace vistle {
namespace insitu {
namespace message {

// after being initialized, sends and receives messages in a blocking manner.
// When the connection is closed returns EngineMEssageType::ConnectionClosed and becomes uninitialized.
// while uninitialized calls to send and received are ignored.
// Received Messages are broadcasted to all ranks so make sure they all call receive together.
class V_INSITUMESSAGEEXPORT InSituTcp {
public:
    template<typename SomeMessage>
    bool send(const SomeMessage &msg) const
    {
        bool error = false;
        if (m_comm.rank() == 0) {
            if (!m_initialized) {
                std::cerr << "InSituTcpMessage uninitialized: can not send message!" << std::endl;
                return false;
            }
            if (msg.type == InSituMessageType::Invalid) {
                std::cerr << "InSituTcpMessage : can not send invalid message!" << std::endl;
                return false;
            }
            boost::system::error_code err;
            InSituMessage ism(msg.type);
            vistle::vecostreambuf<vistle::buffer> buf;
            vistle::oarchive ar(buf);
            ar &msg;
            vistle::buffer vec = buf.get_vector();

            ism.setPayloadSize(vec.size());
            vistle::message::send(*m_socket, ism, err, &vec);
            if (err) {
                error = true;
            }
        }
        return error;
    }
    void initialize(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm);
    bool isInitialized();
    insitu::message::Message recv();

private:
    bool m_initialized = false;
    boost::mpi::communicator m_comm;
    boost::asio::ip::tcp::socket *m_socket;
};
} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !INSITU_TCP_MESSAGE_H
