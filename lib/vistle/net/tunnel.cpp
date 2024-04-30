#include "tunnel.h"
#include <thread>
#include <boost/system/error_code.hpp>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/threadname.h>

#include <vistle/core/messages.h>
#include <vistle/core/tcpmessage.h>
#include <cassert>


namespace asio = boost::asio;
namespace ip = boost::asio::ip;

using std::shared_ptr;

typedef ip::tcp::socket tcp_socket;
typedef ip::tcp::acceptor acceptor;
typedef ip::address_v6 address_v6;

#define CERR std::cerr << "TunnelManager: "

static const size_t BufferSize = 256 * 1024;

namespace vistle {

using message::RequestTunnel;

TunnelManager::TunnelManager()
#if BOOST_VERSION >= 106600
: m_workGuard(asio::make_work_guard(m_io))
#else
: m_workGuard(new asio::io_service::work(m_io))
#endif
{}

TunnelManager::~TunnelManager()
{
    m_workGuard.reset();
    m_io.stop();
    if (io().stopped()) {
        for (auto &t: m_threads)
            t.join();
        m_threads.clear();
        io().reset();
    }
}

void TunnelManager::cleanUp()
{
    for (auto &tun: m_tunnels)
        tun.second->cleanUp();
#if 0
    m_tunnels.clear();

    for (auto &rend: m_rendezvousTunnels) {
        auto &data = rend.second;
        if (data.stream) {
            data.stream->destroy();
            if (data.sock[0] && data.sock[0]->is_open())
                data.sock[0]->close();
            if (data.sock[1] && data.sock[1]->is_open())
                data.sock[1]->close();
            data.stream.reset();
        }
    }
    m_rendezvousTunnels.clear();
#endif
}

void TunnelManager::startThread()
{
    auto &io = m_io;
    auto num = m_threads.size();
    m_threads.emplace_back([&io, num]() {
        setThreadName("tunnel:" + std::to_string(num));
        io.run();
    });
    CERR << "now " << m_threads.size() << " threads in pool" << std::endl;
}

TunnelManager::io_service &TunnelManager::io()
{
    return m_io;
}

bool TunnelManager::processRequest(const message::RequestTunnel &msg)
{
    cleanUp();
    bool ret = false;
    if (msg.remove()) {
        ret = removeTunnel(msg);
    } else {
        ret = addTunnel(msg);
    }
    cleanUp();
    return ret;
}

bool TunnelManager::addSocket(const message::Identify &id, std::shared_ptr<socket> sock0)
{
    assert(id.identity() == message::Identify::TUNNEL);

    // transfer socket to DataProxy's io service
    auto sock = std::make_shared<tcp_socket>(m_io);
    if (sock0->local_endpoint().protocol() == boost::asio::ip::tcp::v4()) {
        sock->assign(boost::asio::ip::tcp::v4(), sock0->release());
    } else if (sock0->local_endpoint().protocol() == boost::asio::ip::tcp::v6()) {
        sock->assign(boost::asio::ip::tcp::v6(), sock0->release());
    } else {
        CERR << "could not transfer socket to io service" << std::endl;
        return false;
    }

    auto tunnelId = id.tunnelId();
    auto streamNo = id.tunnelStreamNumber();
    auto role = id.tunnelRole();
    RendezvousTunnelKey key = {tunnelId, streamNo};
    auto it = m_rendezvousTunnels.find(key);
    if (it == m_rendezvousTunnels.end()) {
        auto &data = m_rendezvousTunnels[key];
        if (role == message::Identify::Server) {
            data.sock[0] = sock;
            CERR << "waiting for client for rendezvous stream " << tunnelId << std::endl;
        } else {
            data.sock[1] = sock;
            CERR << "waiting for server for rendezvous stream " << tunnelId << std::endl;
        }
    } else {
        auto &data = it->second;
        if (data.stream && !data.stream->good()) {
            CERR << "rendezvous tunnel " << tunnelId << ", cleaning up" << std::endl;
            data.stream.reset();
            data.sock[0].reset();
            data.sock[1].reset();
        }
        if (role == message::Identify::Server) {
            if (data.sock[0]) {
                CERR << "rendezvous tunnel " << tunnelId << ", stream " << streamNo << " already have server"
                     << std::endl;
                return false;
            }
            data.sock[0] = sock;
        } else {
            if (data.sock[1]) {
                CERR << "rendezvous tunnel " << tunnelId << ", stream " << streamNo << " already have client"
                     << std::endl;
                return false;
            }
            data.sock[1] = sock;
        }
        if (data.sock[0] && data.sock[1]) {
            CERR << "connected rendezvous stream " << tunnelId << std::endl;
            send(*data.sock[0], message::TunnelEstablished(message::TunnelEstablished::Server));
            send(*data.sock[1], message::TunnelEstablished(message::TunnelEstablished::Client));
            auto stream = std::make_shared<TunnelStream>(data.sock[0], data.sock[1]);
            data.stream = stream;
            stream->start(stream);
            if (m_threads.size() < std::thread::hardware_concurrency())
                startThread();
        }
    }
    return true;
}

bool TunnelManager::addTunnel(const message::RequestTunnel &msg)
{
    assert(!msg.remove());
    assert(msg.destType() != RequestTunnel::Unspecified);
    unsigned short listenPort = msg.srcPort();
    unsigned short destPort = msg.destPort();
    asio::ip::address destAddr;
    if (msg.destIsAddress()) {
        destAddr = msg.destAddr();
    } else {
        const std::string destHost = msg.destHost();
        if (destHost.empty()) {
            CERR << "no destination address for tunnel listening on " << listenPort << std::endl;
            return false;
        }
        asio::ip::tcp::resolver resolver(io());
        auto endpoints = resolver.resolve({destHost, std::to_string(destPort)});
        destAddr = (*endpoints).endpoint().address();
        std::cerr << destHost << " resolved to " << destAddr << std::endl;
    }
    auto it = m_tunnels.find(listenPort);
    if (it != m_tunnels.end()) {
        CERR << "tunnel listening on port " << listenPort << " already exists" << std::endl;
        return false;
    }
    try {
        auto tun = std::make_shared<Tunnel>(*this, listenPort, destAddr, destPort);
        m_tunnels[listenPort] = tun;
        tun->startAccept(tun);
        if (m_threads.size() < std::thread::hardware_concurrency())
            startThread();
    } catch (std::exception &ex) {
        CERR << "error during tunnel creation: " << ex.what() << std::endl;
    }
    return true;
}

bool TunnelManager::removeTunnel(const message::RequestTunnel &msg)
{
    assert(msg.remove());
    unsigned short listenPort = msg.srcPort();
    auto it = m_tunnels.find(listenPort);
    if (it == m_tunnels.end()) {
        CERR << "did not find tunnel listening on port " << listenPort << std::endl;
        return false;
    }
#if 0
   if (it->second->destPort() != msg.destPort()) {
      CERR << "destination port mismatch for tunnel on port " << listenPort << std::endl;
      return false;
   }
   if (it->second->destAddr() != msg.destAddr()) {
      CERR << "destination address mismatch for tunnel on port " << listenPort << std::endl;
      return false;
   }
#endif

    it->second->shutdown();
    m_tunnels.erase(it);
    return true;
}

#undef CERR
#define CERR std::cerr << "Tunnel: "

Tunnel::Tunnel(TunnelManager &manager, unsigned short listenPort, Tunnel::address destAddr, unsigned short destPort)
: m_manager(manager), m_destAddr(destAddr), m_destPort(destPort), m_acceptorv4(manager.io()), m_acceptorv6(manager.io())
{
    boost::system::error_code ec;
    if (!start_listen(listenPort, m_acceptorv4, m_acceptorv6, ec)) {
        CERR << "listening on port " << listenPort << " failed: " << ec.message() << std::endl;
        boost::system::system_error err(ec);
        throw(err);
    }
    CERR << "forwarding connections on port " << listenPort << " to " << m_destAddr << ":" << m_destPort << std::endl;
}

Tunnel::~Tunnel()
{
    shutdown();
}

void Tunnel::shutdown()
{
    for (auto &s: m_listeningSocket) {
        auto &sock = s.second;
        if (sock) {
            if (sock->is_open())
                sock->close();
            boost::system::error_code ec;
            sock->shutdown(asio::socket_base::shutdown_both, ec);
            if (ec) {
                CERR << "error during socket shutdown: " << ec.message() << std::endl;
            }
            sock.reset();
        }
    }
    m_listeningSocket.clear();

    m_acceptorv4.cancel();
    m_acceptorv6.cancel();

    for (auto &s: m_streams) {
        if (std::shared_ptr<TunnelStream> p = s.lock())
            p->destroy();
    }

    cleanUp();
}

void Tunnel::cleanUp()
{
    auto end = m_streams.end();
    for (auto it = m_streams.begin(); it != end; ++it) {
        if (!it->lock()) {
            --end;
            if (it == end) {
                break;
            }
            std::swap(*it, *end);
        }
    }
    m_streams.erase(end, m_streams.end());
}

const Tunnel::address &Tunnel::destAddr() const
{
    return m_destAddr;
}

unsigned short Tunnel::destPort() const
{
    return m_destPort;
}

void Tunnel::startAccept(std::shared_ptr<Tunnel> self)
{
    if (m_acceptorv4.is_open())
        startAccept(m_acceptorv4, self);
    if (m_acceptorv6.is_open())
        startAccept(m_acceptorv6, self);
}

void Tunnel::startAccept(acceptor &a, std::shared_ptr<Tunnel> self)
{
    if (!a.is_open())
        return;
    m_listeningSocket[&a].reset(new tcp_socket(m_manager.io()));
    a.async_accept(*m_listeningSocket[&a],
                   [this, &a, self](boost::system::error_code ec) { handleAccept(a, self, ec); });
}

void Tunnel::handleAccept(acceptor &a, std::shared_ptr<Tunnel> self, const boost::system::error_code &error)
{
    if (error) {
        CERR << "error in accept: " << error.message() << std::endl;
        return;
    }

    std::shared_ptr<tcp_socket> sock0 = m_listeningSocket[&a];
    std::shared_ptr<tcp_socket> sock1(new tcp_socket(m_manager.io()));
    boost::asio::ip::tcp::endpoint dest(m_destAddr, m_destPort);
    CERR << "incoming connection from " << sock0->remote_endpoint() << ", forwarding to " << dest << std::endl;
    sock1->async_connect(
        dest, [this, self, sock0, sock1](boost::system::error_code ec) { handleConnect(self, sock0, sock1, ec); });

    startAccept(a, self);
}

void Tunnel::handleConnect(std::shared_ptr<Tunnel> self, std::shared_ptr<ip::tcp::socket> sock0,
                           std::shared_ptr<ip::tcp::socket> sock1, const boost::system::error_code &error)
{
    if (error) {
        CERR << "error in connect: " << error.message() << std::endl;
        sock0->close();
        sock1->close();
        return;
    }
    CERR << "connected stream..." << std::endl;
    auto stream = std::make_shared<TunnelStream>(sock0, sock1);
    m_streams.emplace_back(stream);
    stream->start(stream);
}

#undef CERR
#define CERR std::cerr << "TunnelStream: "

TunnelStream::TunnelStream(std::shared_ptr<boost::asio::ip::tcp::socket> sock0,
                           std::shared_ptr<boost::asio::ip::tcp::socket> sock1)
{
    m_sock.push_back(sock0);
    m_sock.push_back(sock1);

    for (size_t i = 0; i < m_sock.size(); ++i) {
        m_buf.emplace_back(BufferSize);
    }

    for (size_t i = 0; i < m_sock.size(); ++i) {
        auto &sock = m_sock[i];
        if (!sock->is_open())
            CERR << "TunnelStream: PROBLEM: socket " << i << " not open" << std::endl;
    }
}

TunnelStream::~TunnelStream()
{
    //CERR << "dtor" << std::endl;
    close();
}

void TunnelStream::start(std::shared_ptr<TunnelStream> self)
{
    for (size_t i = 0; i < m_sock.size(); ++i) {
        m_sock[i]->async_read_some(
            asio::buffer(m_buf[i].data(), m_buf[i].size()),
            [this, self, i](boost::system::error_code ec, size_t length) { handleRead(self, i, ec, length); });
    }
}

void TunnelStream::close()
{
    for (auto &sock: m_sock) {
        try {
            if (sock->is_open())
                sock->close();
            boost::system::error_code ec;
            sock->shutdown(asio::socket_base::shutdown_both, ec);
            if (ec) {
                CERR << "error during socket shutdown: " << ec.message() << std::endl;
            }
        } catch (const std::exception &ex) {
            CERR << "caught exception while closing socket: " << ex.what() << std::endl;
        }
    }

    m_good = false;
}

bool TunnelStream::good() const
{
    return m_good;
}

void TunnelStream::destroy()
{
    //CERR << "self destruction" << std::endl;
    close();
}

void TunnelStream::handleRead(std::shared_ptr<TunnelStream> self, size_t sockIdx, boost::system::error_code ec,
                              size_t length)
{
    //CERR << "handleRead:  sockIdx=" << sockIdx << ", len=" << length << std::endl;
    int other = (sockIdx + 1) % 2;
    if (ec) {
        CERR << "read error on socket " << sockIdx << ": " << ec.message() << ", closing stream" << std::endl;
        destroy();
        return;
    }
    //std::cerr << "R" << sockIdx << "(" << length << ")" << std::flush;
    async_write(*m_sock[other], asio::buffer(m_buf[sockIdx].data(), length),
                [this, self, sockIdx](boost::system::error_code error, size_t length) {
                    //int other = (sockIdx+1)%2;
                    //std::cerr << "W" << other << "(" << length << ")" << std::flush;
                    handleWrite(self, sockIdx, error);
                });
}

void TunnelStream::handleWrite(std::shared_ptr<TunnelStream> self, size_t sockIdx, boost::system::error_code ec)
{
    //CERR << "handleWrite: sockIdx=" << sockIdx << std::endl;
    if (ec) {
        CERR << "write error on socket " << sockIdx << ": " << ec.message() << ", closing stream" << std::endl;
        destroy();
        return;
    }
    m_sock[sockIdx]->async_read_some(asio::buffer(m_buf[sockIdx].data(), m_buf[sockIdx].size()),
                                     [this, self, sockIdx](boost::system::error_code error, size_t length) {
                                         handleRead(self, sockIdx, error, length);
                                     });
}

} // namespace vistle
