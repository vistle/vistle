#ifndef INSITU_TCP_MESSAGE_H
#define INSITU_TCP_MESSAGE_H
#include "InSituMessage.h"
#include "MessageHandler.h"
#include "export.h"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <atomic>
namespace vistle {
namespace insitu {
namespace message {

// after being initialized, sends and receives messages in a non-blocking manner.
// When the connection is closed returns EngineMEssageType::ConnectionClosed and becomes uninitialized.
// while uninitialized calls to send and receive are ignored.
// Received Messages are broadcasted to all ranks so make sure they all call receive together.
class V_INSITUMESSAGEEXPORT InSituTcp: public MessageHandler {
public:
    InSituTcp(boost::mpi::communicator comm);
    InSituTcp(boost::mpi::communicator comm, const std::string &ip, unsigned int port);
    ~InSituTcp();
    void startAccept();
    bool waitForConnection();
    unsigned int port() const;
    void setOnConnectedCb(const std::function<void(void)> &cb);
    insitu::message::Message recv() override;
    insitu::message::Message tryRecv() override;
    int socketDescriptor() const;

protected:
private:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef std::lock_guard<std::mutex> Guard;

    boost::mpi::communicator m_comm;

    //boost asio stuff
    boost::asio::io_context m_ioContext;
    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
    std::thread m_ioThread; // thread for io_context
    unsigned short m_port = 31299;
    std::array<std::unique_ptr<boost::asio::ip::tcp::acceptor>, 2> m_acceptors;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::function<void(void)> m_onConnectedCb;
    bool m_isConnected;
    std::atomic_bool m_terminate = false;
    struct Msg {
        InSituMessageType type;
        vistle::buffer buf;
    };
    mutable std::vector<Msg> m_cachedMsgs;
    InSituTcp(boost::mpi::communicator comm, bool dummy);

    bool sendMessage(InSituMessageType type, const vistle::buffer &msg) const override;
    bool start_listen();
};
} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !INSITU_TCP_MESSAGE_H
