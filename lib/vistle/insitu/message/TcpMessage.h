#ifndef INSITU_TCP_MESSAGE_H
#define INSITU_TCP_MESSAGE_H
#include "InSituMessage.h"
#include "export.h"
#include "MessageHandler.h"
namespace vistle {
namespace insitu {
namespace message {

// after being initialized, sends and receives messages in a non-blocking manner.
// When the connection is closed returns EngineMEssageType::ConnectionClosed and becomes uninitialized.
// while uninitialized calls to send and receive are ignored.
// Received Messages are broadcasted to all ranks so make sure they all call receive together.
class V_INSITUMESSAGEEXPORT InSituTcp: public MessageHandler {
public:
    void initialize(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::mpi::communicator comm);
    bool isInitialized();
    insitu::message::Message recv() override;
    insitu::message::Message tryRecv() override;

protected:
private:
    bool sendMessage(InSituMessageType type, const vistle::buffer &msg) const override;
    bool m_initialized = false;
    boost::mpi::communicator m_comm;
    boost::asio::ip::tcp::socket *m_socket;
};
} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !INSITU_TCP_MESSAGE_H
