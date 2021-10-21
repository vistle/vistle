#ifndef INSITU_TCP_MESSAGE_H
#define INSITU_TCP_MESSAGE_H
#include "InSituMessage.h"
#include "export.h"
namespace vistle {
namespace insitu {
namespace message {

// after being initialized, sends and receives messages in a non-blocking manner.
// When the connection is closed returns EngineMEssageType::ConnectionClosed and becomes uninitialized.
// while uninitialized calls to send and receive are ignored.
// Received Messages are broadcasted to all ranks so make sure they all call receive together.
template<typename SomeMessage>
bool send(const SomeMessage &msg, vistle::message::socket_t &sock)
{
    if (msg.type == InSituMessageType::Invalid) {
        std::cerr << "InSituTcpMessage : can not send invalid message!" << std::endl;
        return false;
    }
    InSituMessage ism(msg.type);
    vistle::vecostreambuf<vistle::buffer> buf;
    vistle::oarchive ar(buf);
    ar &msg;
    vistle::buffer vec = buf.get_vector();

    ism.setPayloadSize(vec.size());
    boost::system::error_code err;
    vistle::message::send(sock, ism, err, &vec);
    if (err) {
        return false;
    }
    return true;
}


class V_INSITUMESSAGEEXPORT InSituTcp {
public:
    template<typename SomeMessage>
    bool send(const SomeMessage &msg) const
    {
        if (m_comm.rank() == 0) {
            if (!m_initialized) {
                std::cerr << "InSituTcpMessage uninitialized: can not send message!" << std::endl;
                return false;
            }
            return vistle::insitu::message::send(msg, *m_socket);
        }
        return true; // return values is only meaningful on rank 0;
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
