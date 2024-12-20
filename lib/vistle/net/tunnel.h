#ifndef VISTLE_TUNNEL_H
#define VISTLE_TUNNEL_H

#include <memory>
#include <boost/asio.hpp>
#include <thread>

#include <vistle/core/message.h>
#include <vistle/core/messages.h>

#include "export.h"

namespace vistle {

class TunnelManager;
class TunnelStream;

//! one tunnel waiting for connections
class V_NETEXPORT Tunnel {
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef boost::asio::ip::address address;

public:
    Tunnel(TunnelManager &tunnel, unsigned short listenPort, address destAddr, unsigned short destPort);
    ~Tunnel();
    void startAccept(std::shared_ptr<Tunnel> self);
    void shutdown();

    void cleanUp();
    const address &destAddr() const;
    unsigned short destPort() const;

private:
    TunnelManager &m_manager;
    address m_destAddr;
    unsigned short m_destPort;

    acceptor m_acceptorv4, m_acceptorv6;
    std::map<acceptor *, std::shared_ptr<socket>> m_listeningSocket;
    std::vector<std::weak_ptr<TunnelStream>> m_streams;
    void startAccept(acceptor &a, std::shared_ptr<Tunnel> self);
    void handleAccept(acceptor &a, std::shared_ptr<Tunnel> self, const boost::system::error_code &error);
    void handleConnect(std::shared_ptr<Tunnel> self, std::shared_ptr<boost::asio::ip::tcp::socket> sock0,
                       std::shared_ptr<boost::asio::ip::tcp::socket> sock1, const boost::system::error_code &error);
};

//! a single established connection being tunneled
class V_NETEXPORT TunnelStream {
    typedef boost::asio::ip::tcp::socket socket;

public:
    TunnelStream(std::shared_ptr<boost::asio::ip::tcp::socket> sock0,
                 std::shared_ptr<boost::asio::ip::tcp::socket> sock1);
    ~TunnelStream();
    void start(std::shared_ptr<TunnelStream> self);
    void destroy();
    void close();
    bool good() const;

private:
    std::vector<std::vector<char>> m_buf;
    std::vector<std::shared_ptr<socket>> m_sock;
    void handleRead(std::shared_ptr<TunnelStream> self, size_t sockIdx, boost::system::error_code ec, size_t length);
    void handleWrite(std::shared_ptr<TunnelStream> self, size_t sockIdx, boost::system::error_code ec);
    bool m_good = true;
};

//! manage tunnel creation and destruction
class V_NETEXPORT TunnelManager {
public:
    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef boost::asio::io_context io_context;

    TunnelManager();
    ~TunnelManager();
    io_context &io();

    bool addSocket(const message::Identify &id, std::shared_ptr<socket> sock);

    bool processRequest(const message::RequestTunnel &msg);
    void cleanUp();

private:
    bool addTunnel(const message::RequestTunnel &msg);
    bool removeTunnel(const message::RequestTunnel &msg);
    void startThread();
    io_context m_io;
    std::vector<std::thread> m_threads;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
    std::map<unsigned short, std::shared_ptr<Tunnel>> m_tunnels;
    struct RendezvousTunnelKey {
        std::string id;
        int streamId = 0;
        bool operator<(const RendezvousTunnelKey &other) const
        {
            if (streamId == other.streamId) {
                return id < other.id;
            }
            return streamId < other.streamId;
        }
    };
    struct RendezvousTunnelData {
        std::shared_ptr<socket> sock[2];
        std::shared_ptr<TunnelStream> stream;
    };
    std::map<RendezvousTunnelKey, RendezvousTunnelData> m_rendezvousTunnels;
};

} // namespace vistle
#endif
