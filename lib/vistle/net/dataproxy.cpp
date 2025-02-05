#include "dataproxy.h"
#include <vistle/core/tcpmessage.h>
#include <vistle/core/message.h>
#include <cassert>
#include <vistle/core/statetracker.h>
#include <condition_variable>
#include <boost/asio/deadline_timer.hpp>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/threadname.h>

#define CERR std::cerr << "DataProxy: "

static const bool store_and_forward = true;
static const int min_num_sockets = 2;
static const int max_num_sockets = 12;
static const int connection_timeout = 10; // seconds

namespace asio = boost::asio;
using boost::system::error_code;
typedef std::unique_lock<std::recursive_mutex> lock_guard;


namespace vistle {

using vistle::message::Identify;
using vistle::message::async_send;
using vistle::message::async_forward;
using vistle::message::async_recv;
using vistle::message::async_recv_header;

using std::shared_ptr;

DataProxy::DataProxy(StateTracker &state, unsigned short basePort, bool changePort)
: m_hubId(message::Id::Invalid)
, m_stateTracker(state)
, m_workGuard(asio::make_work_guard(m_io))
, m_port(basePort)
, m_acceptorv4(m_io)
, m_acceptorv6(m_io)
{
    if (m_port == 0) {
        if (!changePort)
            return;
    }

    for (bool connected = false; !connected;) {
        boost::system::error_code ec;
        if (start_listen(m_port, m_acceptorv4, m_acceptorv6, ec)) {
            connected = true;
        } else if (ec == boost::system::errc::address_in_use) {
            if (changePort) {
                ++m_port;
                continue;
            }
        } else {
            CERR << "listening on port " << m_port << " failed" << std::endl;
            boost::system::system_error err(ec);
            throw(err);
        }
    }

    startThread();

    CERR << "proxying data connections through port " << m_port << std::endl;
}

DataProxy::~DataProxy()
{
    m_workGuard.reset();
    io().stop();
    while (!io().stopped()) {
        usleep(10000);
    }

    cleanUp();
}

void DataProxy::setHubId(int id)
{
    m_hubId = id;
    make.setId(m_hubId);
    make.setRank(0);

    if (m_port > 0) {
        startAccept(m_acceptorv4);
        startThread();
        startAccept(m_acceptorv6);
        startThread();
    }
}

void DataProxy::setNumRanks(int size)
{
    m_numRanks = size;
}

void DataProxy::setBoostArchiveVersion(int ver)
{
    if (m_boost_archive_version && m_boost_archive_version != ver) {
        CERR << "local Boost.Archive version mismatch: " << ver << " != " << m_boost_archive_version << std::endl;
    }
    m_boost_archive_version = ver;
}

void DataProxy::setIndexSize(int s)
{
    if (m_indexSize && m_indexSize != s) {
        CERR << "local Index size mismatch: " << s << " != " << m_indexSize << std::endl;
    }
    m_indexSize = s;
}

void DataProxy::setScalarSize(int s)
{
    if (m_scalarSize && m_indexSize != s) {
        CERR << "local Scalar size mismatch: " << s << " != " << m_scalarSize << std::endl;
    }
    m_scalarSize = s;
}

unsigned short DataProxy::port() const
{
    return m_port;
}

void DataProxy::setTrace(message::Type type)
{
    m_traceMessages = type;
}

asio::io_context &DataProxy::io()
{
    return m_io;
}

int DataProxy::idToHub(int id) const
{
    if (id >= message::Id::ModuleBase)
        return m_stateTracker.getHub(id);

    if (id == message::Id::LocalManager || id == message::Id::LocalHub)
        return m_hubId;

    return m_stateTracker.getHub(id);
}

void DataProxy::cleanUp()
{
    if (io().stopped()) {
        if (m_acceptorv4.is_open()) {
            m_acceptorv4.cancel();
            m_acceptorv4.close();
        }
        if (m_acceptorv6.is_open()) {
            m_acceptorv6.cancel();
            m_acceptorv6.close();
        }

        boost::system::error_code ec;
        for (auto &ssv: m_remoteDataSocket) {
            for (auto &s: ssv.second.sockets) {
                s->shutdown(tcp_socket::shutdown_both, ec);
            }
        }
        for (auto &s: m_localDataSocket)
            s.second->shutdown(tcp_socket::shutdown_both, ec);
        for (auto &t: m_threads)
            t.join();
        m_threads.clear();
        m_remoteDataSocket.clear();
        m_localDataSocket.clear();
    }
}

bool DataProxy::answerIdentify(EndPointType type, std::shared_ptr<DataProxy::tcp_socket> sock,
                               const message::Identify &ident)
{
    switch (type) {
    case Local:
        return answerRemoteIdentify(sock, ident);
    case Remote:
        return answerLocalIdentify(sock, ident);
    }

    return false;
}

bool DataProxy::answerLocalIdentify(std::shared_ptr<DataProxy::tcp_socket> sock, const message::Identify &ident)
{
    if (ident.identity() != Identify::LOCALBULKDATA) {
        std::stringstream str;
        str << "invalid identity " << ident.identity() << " connected to local data port";
        shutdownSocket(sock, str.str());
        removeSocket(sock);
        return false;
    }
    if (!ident.verifyMac()) {
        shutdownSocket(sock, "MAC verification failed");
        removeSocket(sock);
        return false;
    }
    return true;
}

bool DataProxy::answerRemoteIdentify(std::shared_ptr<DataProxy::tcp_socket> sock, const message::Identify &ident)
{
    if (ident.identity() == Identify::REQUEST) {
        auto reply = make.message<Identify>(ident, Identify::REMOTEBULKDATA, m_hubId);
        async_send(*sock, reply, nullptr, [sock](error_code ec) {
            if (ec) {
                CERR << "send error" << std::endl;
                return;
            }
        });
        return true;
    } else if (ident.identity() == Identify::REMOTEBULKDATA) {
        if (!ident.verifyMac()) {
            shutdownSocket(sock, "MAC verification failed");
            removeSocket(sock);
            return false;
        }
        if (ident.boost_archive_version() != m_boost_archive_version) {
#ifndef USE_YAS
            std::cerr << "Boost.Archive version on hub " << m_hubId << " is " << m_boost_archive_version << ", but hub "
                      << ident.senderId() << " connected with version " << ident.boost_archive_version() << std::endl;
            if (m_boost_archive_version < ident.boost_archive_version()) {
                std::cerr << "Receiving of remote objects from hub " << ident.senderId() << " will fail" << std::endl;
            } else {
                std::cerr << "Receiving of objects sent to hub " << ident.senderId() << " will fail" << std::endl;
            }
#endif
        }
        if (ident.indexSize() != m_indexSize) {
            std::cerr << "Index size on hub " << m_hubId << " is " << m_indexSize << ", but hub " << ident.senderId()
                      << " uses Index size " << ident.indexSize()
                      << ", transfer is only possible from smaller to larger index size" << std::endl;
        }
        if (ident.scalarSize() != m_scalarSize) {
            std::cerr << "Scalar size on hub " << m_hubId << " is " << m_scalarSize << ", but hub " << ident.senderId()
                      << " uses Scalar size " << ident.scalarSize() << ", expect precision loss" << std::endl;
        }
        return true;
    } else {
        std::stringstream str;
        str << "invalid identity " << ident.identity() << " connected to remote data port";
        shutdownSocket(sock, str.str());
        removeSocket(sock);
        return false;
    }
}

void DataProxy::shutdownSocket(std::shared_ptr<DataProxy::tcp_socket> sock, const std::string &reason)
{
    CERR << "closing socket: " << reason << std::endl;
    auto close = message::CloseConnection(reason);
    send(*sock, close);
}

bool DataProxy::removeSocket(std::shared_ptr<DataProxy::tcp_socket> sock)
{
    sock->shutdown(tcp_socket::shutdown_both);
    sock->close();

    lock_guard lock(m_mutex);

    for (auto it = m_localDataSocket.begin(); it != m_localDataSocket.end(); ++it) {
        if (it->second != sock)
            continue;

        m_localDataSocket.erase(it);
        return true;
    }

    for (auto it = m_remoteDataSocket.begin(); it != m_remoteDataSocket.end(); ++it) {
        auto &socks = it->second.sockets;
        auto it2 = std::find(socks.begin(), socks.end(), sock);
        if (it2 == socks.end())
            continue;

        std::swap(*it2, socks.back());
        socks.pop_back();
        return true;
    }

    return false;
}

void DataProxy::startThread()
{
    lock_guard lock(m_mutex);
    if (true || m_threads.size() < std::thread::hardware_concurrency()) {
        //if (m_threads.size() < 1) {
        auto &io = m_io;
        auto num = m_threads.size();
        m_threads.emplace_back([&io, num]() {
            setThreadName("vistle:data:" + std::to_string(num));
            io.run();
        });
        //CERR << "now " << m_threads.size() << " threads in pool" << std::endl;
    } else {
        CERR << "not starting a new thread, already have " << m_threads.size() << " threads" << std::endl;
    }
}

void DataProxy::startAccept(acceptor &a)
{
    //CERR << "(re-)starting accept" << std::endl;
    std::shared_ptr<tcp_socket> sock(new tcp_socket(io()));
    a.async_accept(*sock, [this, &a, sock](boost::system::error_code ec) { handleAccept(a, ec, sock); });
}

void DataProxy::handleAccept(acceptor &a, const boost::system::error_code &error, std::shared_ptr<tcp_socket> sock)
{
    if (error) {
        CERR << "error in accept: " << error.message() << std::endl;
        return;
    }

    startAccept(a);

    startThread();
    startThread();

    auto ident = make.message<Identify>();
    if (message::send(*sock, ident)) {
        //CERR << "sent ident msg to remote, sock.use_count()=" << sock.use_count() << std::endl;
    }

    using namespace message;

    std::shared_ptr<message::Buffer> buf(new message::Buffer);
    message::async_recv(*sock, *buf, [this, sock, buf](const error_code ec, std::shared_ptr<buffer> payload) {
        if (ec) {
            CERR << "recv error after accept: " << ec.message() << ", sock.use_count()=" << sock.use_count()
                 << std::endl;
            return;
        }
        if (payload && payload->size() > 0) {
            CERR << "recv error after accept: cannot handle payload of size " << payload->size() << std::endl;
        }

        if (m_traceMessages == message::ANY || buf->type() == m_traceMessages) {
            CERR << "handle accept: " << *buf << std::endl;
        }

        //CERR << "received initial message on incoming connection: type=" << buf->type() << std::endl;
        switch (buf->type()) {
        case message::IDENTIFY: {
            auto &id = buf->as<message::Identify>();
            serveSocket(id, sock);
            break;
        }
        default: {
            std::stringstream str;
            str << "expected Identify message, got " << buf->type() << std::endl;
            str << "got: " << *buf;
            shutdownSocket(sock, str.str());
        }
        }
    });
}

bool DataProxy::serveSocket(const message::Identify &id, std::shared_ptr<tcp_socket> sock)
{
    switch (id.identity()) {
    case Identify::REMOTEBULKDATA: {
        if (!id.verifyMac()) {
            shutdownSocket(sock, "MAC verification failed");
            return false;
        }

        {
            lock_guard lock(m_mutex);
            auto &socks = m_remoteDataSocket[id.senderId()].sockets;
            if (std::find(socks.begin(), socks.end(), sock) != socks.end()) {
                return false;
            }
            socks.emplace_back(sock);
            std::cerr << "." << std::flush;
        }

        if (id.boost_archive_version() != m_boost_archive_version) {
#ifndef USE_YAS
            std::cerr << "Boost.Archive version on hub " << m_hubId << " is " << m_boost_archive_version << ", but hub "
                      << id.senderId() << " connected with version " << id.boost_archive_version() << std::endl;
            if (m_boost_archive_version < id.boost_archive_version()) {
                std::cerr << "Receiving of remote objects from hub " << id.senderId() << " will fail" << std::endl;
            } else {
                std::cerr << "Receiving of objects sent to hub " << id.senderId() << " will fail" << std::endl;
            }
#endif
        }

        msgForward(sock, Local);
        break;
    }
    case Identify::LOCALBULKDATA: {
        if (!id.verifyMac()) {
            shutdownSocket(sock, "MAC verification failed");
            return false;
        }

        {
            lock_guard lock(m_mutex);
            m_localDataSocket[id.rank()] = sock;
        }

        msgForward(sock, Remote);
        break;
    }
    default: {
        std::stringstream str;
        str << "unexpected identity " << id.identity();
        shutdownSocket(sock, str.str());
        return false;
        break;
    }
    }

    return true;
}

void DataProxy::msgForward(std::shared_ptr<tcp_socket> sock, EndPointType type)
{
    using namespace vistle::message;

    std::shared_ptr<message::Buffer> msg(new message::Buffer);
    if (store_and_forward) {
        async_recv(*sock, *msg, [this, sock, msg, type](error_code ec, std::shared_ptr<buffer> payload) {
            if (ec) {
                CERR << "msgForward, dest=" << toString(type) << ": error " << ec.message() << std::endl;
                return;
            }

            if (m_traceMessages == message::ANY || msg->type() == m_traceMessages) {
                CERR << "handle forward to " << toString(type) << ": " << *msg << std::endl;
            }

            if (msg->type() == IDENTIFY) {
                auto &ident = msg->as<const Identify>();
                if (!answerIdentify(type, sock, ident)) {
                    return;
                }
            }

            if (msg->type() == CLOSECONNECTION) {
                auto &close = msg->as<CloseConnection>();
                CERR << "peer is closing connection: " << close.reason() << std::endl;
                removeSocket(sock);
                return;
            }

            msgForward(sock, type);

            bool needPayload = false;
            bool forward = false;
            switch (msg->type()) {
            case IDENTIFY: {
                // already handled
                break;
            }
            case SENDOBJECT: {
                forward = true;
                needPayload = true;
#ifdef DEBUG
                auto &send = msg->as<const SendObject>();
                if (send.isArray()) {
                    CERR << "local SendObject: array " << send.objectId() << ", size=" << send.payloadSize()
                         << std::endl;
                } else {
                    CERR << "local SendObject: object " << send.objectId() << ", size=" << send.payloadSize()
                         << std::endl;
                }
#endif
                break;
            }
            case REQUESTOBJECT: {
                forward = true;
                break;
            }
            case ADDOBJECTCOMPLETED: {
                forward = true;
                break;
            }
            case REMOTERENDERING: {
                forward = true;
                needPayload = true;
                break;
            }
            default: {
                CERR << "have unexpected message " << *msg << std::endl;
                forward = true;
                break;
            }
            }

            if (!needPayload) {
                if (payload) {
                    CERR << "have unnecessary payload for " << *msg << std::endl;
                }
                needPayload = true;
            }

            if (forward) {
                auto dest = getDataSock(*msg);
                if (dest) {
                    async_send(*dest, *msg, payload, [type, dest, payload](error_code ec) mutable {
                        message::return_buffer(payload);
                        if (ec) {
                            CERR << "error in write to " << toString(type) << ": " << ec.message() << std::endl;
                            return;
                        }
                    });
                } else {
                    CERR << "no destination socket for " << toString(type) << " forward of " << *msg << std::endl;
                }
            } else {
                message::return_buffer(payload);
            }
        });
    } else {
        async_recv_header(*sock, *msg, [this, sock, msg, type](error_code ec) {
            if (ec) {
                CERR << "msgForward, dest=" << toString(type) << ": error " << ec.message() << std::endl;
                return;
            }

            if (m_traceMessages == message::ANY || msg->type() == m_traceMessages) {
                CERR << "handle forward to " << toString(type) << ": " << *msg << std::endl;
            }

            bool forward = false;
            switch (msg->type()) {
            case IDENTIFY: {
                auto &ident = msg->as<const Identify>();
                if (!answerIdentify(type, sock, ident)) {
                    return;
                }
                break;
            }
            case CLOSECONNECTION: {
                auto &close = msg->as<CloseConnection>();
                CERR << "peer is closing connection: " << close.reason() << std::endl;
                removeSocket(sock);
                return;
                break;
            }
            case SENDOBJECT: {
                forward = true;
#ifdef DEBUG
                auto &send = msg->as<const SendObject>();
                if (send.isArray()) {
                    CERR << "local SendObject: array " << send.objectId() << ", size=" << send.payloadSize()
                         << std::endl;
                } else {
                    CERR << "local SendObject: object " << send.objectId() << ", size=" << send.payloadSize()
                         << std::endl;
                }
#endif
                break;
            }
            case REQUESTOBJECT: {
                forward = true;
                break;
            }
            case ADDOBJECTCOMPLETED: {
                forward = true;
                break;
            }
            case REMOTERENDERING: {
                forward = true;
                break;
            }
            default: {
                CERR << "have unexpected message " << *msg << std::endl;
                forward = true;
                break;
            }
            }

            if (forward) {
                auto dest = getDataSock(*msg);
                if (dest) {
                    if (msg->payloadSize() > 0) {
                        //std::cerr << "msg: " << *msg << ", forwarding " << toString(type) << " payload of " << msg->payloadSize() << " bytes" << std::endl;
                        async_forward(*dest, *msg, sock, [this, type, dest, sock](error_code ec) mutable {
                            if (ec) {
                                CERR << "error in write to " << toString(type) << ": " << ec.message() << std::endl;
                                return;
                            }

                            msgForward(sock, type);
                        });
                    } else {
                        msgForward(sock, type);

                        async_send(*dest, *msg, nullptr, [type, dest](error_code ec) mutable {
                            if (ec) {
                                CERR << "error in write to " << toString(type) << ": " << ec.message() << std::endl;
                                return;
                            }
                        });
                    }
                } else {
                    CERR << "no destination socket for " << toString(type) << " forward of " << *msg << std::endl;
                }
            } else {
                msgForward(sock, type);
            }
        });
    }
}

void DataProxy::printConnections() const
{
    CERR << "CONNECTIONS:" << std::endl;

    for (auto &p: m_localDataSocket) {
        auto &s = *p.second;
        CERR << "LOCAL: " << s.local_endpoint() << " -> " << s.remote_endpoint() << std::endl;
    }

    for (auto &p: m_remoteDataSocket) {
        int i = 0;
        for (auto &s: p.second.sockets) {
            CERR << "REMOTE " << i++ << ": " << s->local_endpoint() << " -> " << s->remote_endpoint() << std::endl;
        }
    }
}

bool DataProxy::connectRemoteData(const message::AddHub &remote)
{
    CERR << "connectRemoteData: " << remote << std::endl;

    if (remote.id() == m_hubId)
        return true;

    if (!message::Id::isHub(remote.id())) {
        CERR << "id is not for a hub: " << remote.id() << std::endl;
        return false;
    }

    if (remote.hasAddress()) {
        if (remote.address().is_unspecified()) {
            CERR << "remote address is unspecified" << std::endl;
            return false;
        }
    } else {
        if (remote.host().empty()) {
            CERR << "remote host is empty" << std::endl;
            return false;
        }
    }

    auto hubId = remote.id();

    auto connectingSockets = std::make_shared<std::set<std::shared_ptr<tcp_socket>>>();

    asio::deadline_timer timer(io());
    timer.expires_from_now(boost::posix_time::seconds(connection_timeout));
    timer.async_wait([this, hubId, connectingSockets](const boost::system::error_code &ec) {
        if (ec == asio::error::operation_aborted) {
            // timer was canceled
            return;
        }
        if (ec) {
            CERR << "timer failed: " << ec.message() << std::endl;
            lock_guard lock(m_mutex);
            connectingSockets->clear();
            return;
        }

        CERR << "timeout for bulk data connection to " << hubId << std::endl;
        lock_guard lock(m_mutex);
        for (auto &s: *connectingSockets) {
            boost::system::error_code ec;
            s->cancel(ec);
            if (ec) {
                CERR << "cancelling operations on socket failed: " << ec.message() << std::endl;
            } else {
                bool open = s->is_open();
                s->close(ec);
                if (ec) {
                    if (open) {
                        CERR << "closing socket failed: " << ec.message() << std::endl;
                    }
                }
            }
        }
        connectingSockets->clear();
    });

    unsigned short dataPort = remote.dataPort() ? remote.dataPort() : remote.port();

    size_t numconn = std::min(max_num_sockets, std::max(min_num_sockets, std::max(m_numRanks, remote.numRanks()) + 4));
    size_t numtries = numconn - m_remoteDataSocket[hubId].sockets.size();
    CERR << "establishing data connection from hub " << m_hubId << " with " << m_numRanks << " ranks to " << remote.id()
         << " with " << remote.numRanks() << " ranks, " << numtries << " tries for " << numconn
         << " parallel connections to " << remote.address() << ":" << dataPort << std::flush;
    boost::asio::ip::tcp::endpoint dest(remote.address(), dataPort);

    size_t count = 0;
    lock_guard lock(m_mutex);
    while (m_remoteDataSocket[hubId].sockets.size() < numconn && count < numtries) {
        ++count;
        auto sock = std::make_shared<asio::ip::tcp::socket>(io());
        connectingSockets->insert(sock);
        lock.unlock();

        sock->async_connect(
            dest, [this, sock, remote, dataPort, hubId, connectingSockets](const boost::system::error_code &ec) {
                lock_guard lock(m_mutex);
                connectingSockets->erase(sock);

                if (ec == asio::error::operation_aborted) {
                    //CERR << "connecting to " << remote.id() << " was aborted" << std::endl;
                    return;
                }
                if (ec == asio::error::timed_out) {
                    CERR << "connecting to " << remote.id() << "/" << remote.address() << ":" << dataPort
                         << " timed out" << std::endl;
                    return;
                }
                if (ec) {
                    CERR << "could not establish bulk data connection to " << remote.id() << "/" << remote.address()
                         << ":" << dataPort << ": " << ec.message() << std::endl;
                    return;
                }

                auto &socks = m_remoteDataSocket[hubId].sockets;
                if (std::find(socks.begin(), socks.end(), sock) != socks.end()) {
                    return;
                }

                m_remoteDataSocket[hubId].sockets.emplace_back(sock);
                lock.unlock();
                std::cerr << "." << std::flush;
                //CERR << "connected to " << remote.address << ":" << dataPort << ", now have " << m_remoteDataSocket[hubId].sockets.size() << " connections" << std::endl;

                startThread();
                startThread();
                msgForward(sock, Local);
            });

        lock.lock();
    }

    while (!connectingSockets->empty() && m_remoteDataSocket[hubId].sockets.size() < numconn) {
        lock.unlock();
        usleep(10000);
        lock.lock();
    }

    timer.cancel();
    connectingSockets->clear();

    std::cerr << std::endl;
    if (m_remoteDataSocket[hubId].sockets.empty()) {
        CERR << "WARNING: could not establish data connection to " << remote.address() << ":" << dataPort << std::endl;
        m_remoteDataSocket.erase(hubId);

        return false;
    }

    if (m_remoteDataSocket[hubId].sockets.size() >= numconn) {
        CERR << "connected to hub " << hubId << " at " << remote.address() << ":" << dataPort << " with "
             << m_remoteDataSocket[hubId].sockets.size() << " parallel connections" << std::endl;
    } else {
        CERR << "WARNING: connected to hub (data) at " << remote.address() << ":" << dataPort << " with ONLY "
             << m_remoteDataSocket[hubId].sockets.size() << " parallel connections" << std::endl;
    }

    //printConnections();

    return true;
}

bool DataProxy::addSocket(const message::Identify &id, std::shared_ptr<DataProxy::tcp_socket> sock)
{
    using message::Identify;

    startThread();
    startThread();

    // transfer socket to DataProxy's io context
    auto sock2 = std::make_shared<tcp_socket>(m_io);
    if (sock->local_endpoint().protocol() == boost::asio::ip::tcp::v4()) {
        sock2->assign(boost::asio::ip::tcp::v4(), sock->release());
    } else if (sock->local_endpoint().protocol() == boost::asio::ip::tcp::v6()) {
        sock2->assign(boost::asio::ip::tcp::v6(), sock->release());
    } else {
        CERR << "could not transfer socket to io context" << std::endl;
        return false;
    }

    return serveSocket(id, sock2);
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getLocalDataSock(const message::Message &msg)
{
    lock_guard lock(m_mutex);
    auto it = m_localDataSocket.find(msg.destRank());
    if (it == m_localDataSocket.end()) {
        CERR << "did not find local destination socket for " << msg << std::endl;
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getRemoteDataSock(const message::Message &msg)
{
    int hubId = idToHub(msg.destId());

    lock_guard lock(m_mutex);
    auto it = m_remoteDataSocket.find(hubId);
#if 0
   if (it == m_remoteDataSocket.end()) {
      if (!connectRemoteData(hubId)) {
         CERR << "failed to establish data connection to remote hub with id " << hubId << std::endl;
         return std::shared_ptr<asio::ip::tcp::socket>();
      }
   }
   it = m_remoteDataSocket.find(hubId);
#endif
    if (it == m_remoteDataSocket.end() || it->second.sockets.empty()) {
        if (hubId != message::Id::MasterHub && m_hubId != message::Id::MasterHub) {
            it = m_remoteDataSocket.find(message::Id::MasterHub);
            if (it == m_remoteDataSocket.end()) {
                CERR << "did not find remote destination socket for master and for " << msg << std::endl;
                return nullptr;
            }
        } else {
            CERR << "did not find remote destination socket for " << msg << std::endl;
            return nullptr;
        }
    }
    auto &socks = it->second.sockets;
    if (socks.empty()) {
        CERR << "no remote destination socket connected for " << msg << std::endl;
        return nullptr;
    }
    auto &idx = it->second.next_socket;
    idx %= socks.size();
    return socks[idx++];
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getDataSock(const message::Message &msg)
{
    if (idToHub(msg.destId()) == m_hubId) {
        return getLocalDataSock(msg);
    }

    return getRemoteDataSock(msg);
}

} // namespace vistle
