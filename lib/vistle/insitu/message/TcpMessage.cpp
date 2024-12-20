#include "TcpMessage.h"
#include <vistle/insitu/core/slowMpi.h>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <vistle/util/listenv4v6.h>
#include <vistle/insitu/core/exception.h>

using namespace vistle::insitu;
using namespace vistle::insitu::message;
namespace asio = boost::asio;


InSituTcp::InSituTcp(boost::mpi::communicator comm): InSituTcp(comm, false)
{
    if (m_comm.rank() == 0) {
        m_acceptors[0] = std::make_unique<asio::ip::tcp::acceptor>(m_ioContext);
        m_acceptors[1] = std::make_unique<asio::ip::tcp::acceptor>(m_ioContext);
        if (start_listen()) {
            startAccept();
        }
    }
}


InSituTcp::InSituTcp(boost::mpi::communicator comm, bool dummy)
: m_comm(comm, boost::mpi::comm_create_kind::comm_duplicate)
, m_workGuard(boost::asio::make_work_guard(m_ioContext))
, m_ioThread([this]() { m_ioContext.run(); })
{}

InSituTcp::InSituTcp(boost::mpi::communicator comm, const std::string &ip, unsigned int port): InSituTcp(comm, false)
{
    if (m_comm.rank() == 0) {
        boost::system::error_code ec;
        asio::ip::tcp::resolver resolver(m_ioContext);
        m_socket.reset(new socket(m_ioContext));
        auto endpoints = resolver.resolve(ip, std::to_string(port), asio::ip::tcp::resolver::numeric_service, ec);
        if (ec) {
            throw insitu::InsituException() << "initializeEngineSocket failed to resolve connect socket";
        }

        asio::connect(*m_socket, endpoints, ec);

        if (ec) {
            throw insitu::InsituException() << "initializeEngineSocket failed to connect socket";
        }
    }
}

InSituTcp::~InSituTcp()
{
    m_terminate = true;
    for (auto &a: m_acceptors)
        if (a)
            a->close();
    if (m_socket)
        m_socket->close();
    m_ioContext.stop();
    m_ioThread.join();
}

unsigned int InSituTcp::port() const
{
    return m_port;
}

void InSituTcp::setOnConnectedCb(const std::function<void(void)> &cb)
{
    m_onConnectedCb = cb;
}

void InSituTcp::startAccept()
{
    if (!m_acceptors[0]->is_open() || !m_acceptors[1]->is_open())
        return;
    for (size_t i = 0; i < m_acceptors.size(); i++) {
        auto sock = new boost::asio::ip::tcp::socket{m_ioContext};
        m_acceptors[i]->async_accept(*sock, [this, i, sock](boost::system::error_code ec) {
            if (!ec) {
                std::lock_guard<std::mutex> g{m_mutex};
                m_socket.reset(sock);
                m_acceptors[!i]->close(); //close the other one
                if (m_onConnectedCb)
                    m_onConnectedCb();
                for (const auto &msg: m_cachedMsgs)
                    sendMessage(msg.type, msg.buf);
                m_cachedMsgs.clear();
                m_cv.notify_all();

            } else {
                delete sock;
            }
        });
    }
}

bool InSituTcp::waitForConnection()
{
    if (m_comm.rank() == 0) {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait(lk);
        m_isConnected = m_socket != nullptr;
    }
    vistle::insitu::broadcast(m_comm, m_isConnected, 0, false, m_terminate);

    return m_isConnected;
}


bool InSituTcp::start_listen()
{
    boost::system::error_code ec;
    unsigned short port = m_port;
    while (!vistle::start_listen(port, *m_acceptors[0], *m_acceptors[1], ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else if (ec) {
            if (ec != boost::system::errc::operation_canceled)
                std::cerr << "failed to listen on port " << port << ": " << ec.message() << std::endl;
            return false;
        }
    }
    m_port = port;
    return true;
}

bool InSituTcp::sendMessage(InSituMessageType type, const vistle::buffer &vec) const
{
    if (!m_socket) {
        m_cachedMsgs.push_back({type, vec});
        return true;
    }

    InSituMessage ism(type);
    ism.setPayloadSize(vec.size());

    boost::system::error_code err;
    vistle::message::send(*m_socket, ism, err, &vec);
    if (err) {
        return false;
    }
    return true;
}

Message InSituTcp::recv()
{
    vistle::buffer payload;
    bool error = false;

    int type;

    if (m_comm.rank() == 0) {
        boost::system::error_code err;
        vistle::message::Buffer bf;
        if (!m_socket)
            error = true;
        else if (!vistle::message::recv(*m_socket, bf, err, false, &payload)) {
            error = true;
        } else if (err || static_cast<vistle::message::Message &>(bf).type() != vistle::message::Type::INSITU) {
            type = static_cast<int>(InSituMessageType::ConnectionClosed);
            ConnectionClosed proxy{false};
            vistle::vecostreambuf<vistle::buffer> buf;
            vistle::oarchive ar(buf);
            ar &proxy;
            payload = buf.get_vector();
        } else {
            type = static_cast<int>(bf.as<InSituMessage>().ismType());
        }
    }
    vistle::insitu::broadcast(m_comm, error, 0, true, m_terminate);
    if (error) {
        return Message::errorMessage();
    }
    boost::mpi::broadcast(m_comm, type, 0);
    if (type == static_cast<int>(InSituMessageType::ConnectionClosed)) {
        m_socket->close();
        m_socket = nullptr;
    }
    int size = payload.size();
    boost::mpi::broadcast(m_comm, size, 0);
    if (m_comm.rank() != 0) {
        payload.resize(size);
    }
    boost::mpi::broadcast(m_comm, payload.data(), payload.size(), 0);
    return Message{static_cast<InSituMessageType>(type), std::move(payload)};
}

Message InSituTcp::tryRecv()
{
    return recv();
}

int InSituTcp::socketDescriptor() const
{
    return m_socket ? m_socket->native_handle() : 0;
}
