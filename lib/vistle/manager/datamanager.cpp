#include <boost/mpi/communicator.hpp>

#include "datamanager.h"
#include "clustermanager.h"
#include "communicator.h"
#include <vistle/util/vecstreambuf.h>
#include <vistle/util/sleep.h>
#include <vistle/util/threadname.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/core/archives.h>
#include <vistle/core/archive_loader.h>
#include <vistle/core/archive_saver.h>
#include <vistle/core/archives_impl.h>
#include <vistle/core/shm_array_impl.h>
#include <vistle/core/statetracker.h>
#include <vistle/core/object.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/messages.h>
#include <vistle/core/shmvector.h>
#include <vistle/config/value.h>
#include <vistle/config/access.h>
#include <iostream>
#include <functional>

#define CERR std::cerr << "data [" << m_rank << "/" << m_size << "] "

//#define DEBUG

namespace asio = boost::asio;
namespace mpi = boost::mpi;

namespace vistle {

namespace {

bool isLocal(int id)
{
    auto &comm = Communicator::the();
    auto &state = comm.clusterManager().state();
    return (state.getHub(id) == comm.hubId());
}

} // namespace

DataManager::DataManager(mpi::communicator &comm, unsigned short baseport)
: m_comm(comm, mpi::comm_duplicate)
, m_rank(m_comm.rank())
, m_size(m_comm.size())
, m_port(baseport)
, m_acceptorv4(m_ioContext)
, m_acceptorv6(m_ioContext)
, m_dataSocket(std::make_shared<asio::ip::tcp::socket>(m_ioContext))
, m_workGuard(asio::make_work_guard(m_ioContext))
, m_recvThread([this]() {
    setThreadName("vistle:dmgr_recv");
    recvLoop();
})
, m_cleanThread([this]() {
    setThreadName("vistle:dmgr_clean");
    cleanLoop();
})
{
    vistle::config::Access config;
    m_connectionTimeout = *config.value<double>("system", "net", "connection_timeout", m_connectionTimeout);
    m_numConnections = *config.value<int64_t>("system", "net", "num_direct_connections", m_numConnections);
    m_useDirectComm = m_numConnections > 0;
    m_asyncSend = *config.value<bool>("system", "net", "async_send", m_asyncSend);

    m_ioThreads.emplace_back([this]() {
        setThreadName("vistle:dmgr_send");
        sendLoop();
    });

    boost::system::error_code ec;
    while (!start_listen(m_port, m_acceptorv4, m_acceptorv6, ec)) {
        if (ec != boost::asio::error::address_in_use) {
            m_port = 0;
            break;
        }
        ++m_port;
    }
    if (m_port != 0) {
        CERR << "data manager on rank " << m_rank << " listening on port " << m_port << std::endl;
        for (auto *a: {&m_acceptorv4, &m_acceptorv6}) {
            startAccept(*a);
            startThread();
        }
        m_listenThread = std::thread([this]() {
            setThreadName("vistle:dmgr_listen");
            listenLoop();
        });
    }

    if (m_size > 1)
        m_req = m_comm.irecv(boost::mpi::any_source, Communicator::TagData, &m_msgSize, 1);
}

DataManager::~DataManager()
{
    {
        std::lock_guard<std::mutex> guard(m_recvMutex);
        m_quit = true;
    }

    if (m_size > 1)
        m_req.cancel();
    m_req.wait();

    for (auto &a: {&m_acceptorv4, &m_acceptorv6}) {
        boost::system::error_code ec;
        a->close(ec);
        if (ec) {
            CERR << "error closing acceptor: " << ec.message() << std::endl;
        }
    }

    closeAllDirect();
    if (m_dataSocket && m_dataSocket->is_open()) {
        boost::system::error_code ec;
        m_dataSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec) {
            CERR << "error during data socket shutdown: " << ec.message() << std::endl;
        }
        m_dataSocket->close(ec);
        if (ec) {
            CERR << "error during data socket shutdown: " << ec.message() << std::endl;
        }
    }
    m_dataSocket.reset();

    m_recvThread.join();
    if (m_listenThread.joinable())
        m_listenThread.join();
    m_cleanThread.join();

    m_workGuard.reset();
    m_ioContext.stop();

    std::lock_guard guard(m_threadsMutex);
    for (auto &t: m_ioThreads) {
        if (t.joinable())
            t.join();
    }
}

unsigned short DataManager::port() const
{
    return m_port;
}

DataManager::io_context &DataManager::io()
{
    return m_ioContext;
}

void DataManager::startThread()
{
    std::lock_guard guard(m_threadsMutex);
    if (true || m_ioThreads.size() < std::thread::hardware_concurrency()) {
        auto &io = m_ioContext;
        auto num = m_ioThreads.size();
        m_ioThreads.emplace_back([&io, num]() {
            setThreadName("vistle:dmgr_io:" + std::to_string(num));
            io.run();
        });
        //CERR << "now " << m_ioThreads.size() << " threads in pool" << std::endl;
    } else {
#ifdef DEBUG
        CERR << "not starting a new thread, already have " << m_ioThreads.size() << " threads" << std::endl;
#endif
    }
}

void DataManager::startAccept(acceptor &a)
{
    //CERR << "(re-)starting accept" << std::endl;
    std::shared_ptr<tcp_socket> sock(new tcp_socket(io()));
    a.async_accept(*sock, [this, &a, sock](boost::system::error_code ec) { handleAccept(a, ec, sock); });
}

void DataManager::handleAccept(acceptor &a, const boost::system::error_code &error, std::shared_ptr<tcp_socket> sock)
{
    if (error) {
        CERR << "error in accept: " << error.message() << std::endl;
        return;
    }

    startAccept(a);
    startThread();

    std::string name("dmgr:");
    name += std::to_string(Communicator::the().hubId());
    name += ":" + std::to_string(m_rank);
    message::Identify ident(name);
    if (message::send(*sock, ident)) {
#ifdef DEBUG
        CERR << "sent ident msg to remote, sock.use_count()=" << sock.use_count() << std::endl;
#endif
    }

    std::shared_ptr<message::Buffer> buf(new message::Buffer);
    message::async_recv(
        *sock, *buf, [this, sock, buf](const boost::system::error_code ec, std::shared_ptr<buffer> payload) {
            if (ec) {
                CERR << "recv error after accept: " << ec.message() << ", sock.use_count()=" << sock.use_count()
                     << std::endl;
                return;
            }

#ifdef DEBUG
            CERR << "received initial message on incoming connection: type=" << buf->type() << std::endl;
#endif
            switch (buf->type()) {
            case message::IDENTIFY: {
                auto &id = buf->as<message::Identify>();
                switch (id.identity()) {
                case message::Identify::DIRECTBULKDATA: {
                    if (!id.verifyMac()) {
                        shutdownSocket(sock, "MAC verification failed");
                        return;
                    }
                    serveDirectSocket(sock);
                    break;
                }
                default: {
                    std::stringstream str;
                    str << "unexpected identity " << id.identity();
                    shutdownSocket(sock, str.str());
                    return;
                }
                }
                break;
            }
            default: {
                std::stringstream str;
                str << "expected Identify message, got " << buf->type() << std::endl;
                str << "got: " << *buf;
                shutdownSocket(sock, str.str());
                return;
            }
            }
        });
}

void DataManager::shutdownSocket(std::shared_ptr<DataManager::tcp_socket> sock, const std::string &reason)
{
#ifdef DEBUG
    CERR << "closing socket: " << reason << std::endl;
#endif
    auto close = message::CloseConnection(reason);
    message::send(*sock, close);
}

bool DataManager::serveDirectSocket(std::shared_ptr<tcp_socket> sock)
{
    for (;;) {
        if (!sock->is_open()) {
            return false;
        }

        bool gotMsg = false;
        message::Buffer buf;
        buffer payload;
        message::error_code ec;
        if (message::recv(*sock, buf, ec, false, &payload)) {
            {
                std::lock_guard<std::mutex> guard(m_recvMutex);
                if (m_quit)
                    break;
            }
            gotMsg = true;
            handle(sock, buf, &payload);
        } else if (ec) {
            CERR << "Data communication error: " << ec.message() << std::endl;
        }

        {
            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }

        vistle::adaptive_wait(gotMsg, sock.get());

        std::lock_guard<std::mutex> guard(m_recvMutex);
        if (m_quit)
            break;
    }

    return true;
}

bool DataManager::connect(boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> &hub)
{
#ifdef DEBUG
    CERR << "connecting to local hub..." << std::endl;
#endif

    boost::system::error_code ec;
    asio::connect(*m_dataSocket, hub, ec);
    if (ec) {
        CERR << "could not establish bulk data connection to local hub on rank " << m_rank << std::endl;
        return false;
    }
#ifdef DEBUG
    CERR << "connected to local hub" << std::endl;
#endif

    return true;
}

bool DataManager::addHub(const message::AddHub &hub, const message::AddHub::Payload &payload)
{
    std::unique_lock<std::mutex> lock(m_directSocketsMutex);
    auto numRanks = payload.rankAddresses.size();
    if (m_directSockets.find(hub.id()) != m_directSockets.end()) {
        CERR << "already have direct connection to hub " << hub.id() << std::endl;
        return true;
    }
#ifdef DEBUG
    CERR << "connecting to hub " << hub << std::endl;
#endif
    auto &socketLists = m_directSockets[hub.id()];
    auto &rankSockets = socketLists;

    for (size_t i = 0; i < numRanks; ++i) {
        auto port = payload.rankDataPorts[i];
        if (port == 0) {
            CERR << "WARNING: hub " << hub.id() << " has no data manager port for rank " << i << std::endl;
            continue;
        }
        std::vector<boost::asio::ip::tcp::endpoint> endpoints;
        for (auto it = payload.rankAddresses[i].begin(); it != payload.rankAddresses[i].end(); ++it) {
            auto addr = boost::asio::ip::make_address(*it);
            endpoints.push_back(boost::asio::ip::tcp::endpoint(addr, port));
        }
        rankSockets.emplace_back(io());

#ifdef DEBUG
        CERR << "connecting to hub " << hub.id() << " at port " << port << " for rank " << i << std::endl;
#endif
        auto &socketList = rankSockets[i];
        auto &timer = socketList.connectionTimer;
        timer.expires_from_now(boost::posix_time::seconds(static_cast<int>(std::ceil(m_connectionTimeout))));
        auto sock = std::make_shared<boost::asio::ip::tcp::socket>(io());
        socketList.connecting.insert(sock);
        timer.async_wait([this, hub, &socketList](const boost::system::error_code &ec) {
            if (ec == asio::error::operation_aborted) {
                // timer was canceled
                return;
            }

            if (ec) {
                CERR << "timer failed: " << ec.message() << std::endl;
            } else {
                CERR << "timeout for bulk data connection to " << hub.id() << std::endl;
            }

            std::lock_guard lock(socketList.mutex);
            for (auto &s: socketList.connecting) {
                boost::system::error_code ec;
                s->cancel(ec);
                if (ec) {
                    CERR << "cancelling operations on socket failed: " << ec.message() << std::endl;
                } else {
                    s->shutdown(tcp_socket::shutdown_both, ec);
                    bool open = s->is_open();
                    s->close(ec);
                    if (ec) {
                        if (open) {
                            CERR << "closing socket failed: " << ec.message() << std::endl;
                        }
                    }
                }
            }
            socketList.connecting.clear();
        });

        std::unique_lock lock(socketList.mutex);
        size_t count = 0;
        size_t numtries = m_numConnections - socketList.sockets.size();
        while (socketList.sockets.size() < m_numConnections && count < numtries) {
            ++count;
            auto sock = std::make_shared<asio::ip::tcp::socket>(io());
            socketList.connecting.insert(sock);
            lock.unlock();

            async_connect(*sock, endpoints,
                          [this, sock, hub, port, &socketList](const boost::system::error_code &ec,
                                                               const asio::ip::tcp::endpoint &endpoint) {
                              std::unique_lock lock(socketList.mutex);
                              socketList.connecting.erase(sock);

                              if (ec == asio::error::operation_aborted) {
                                  return;
                              }
                              if (ec == asio::error::timed_out) {
                                  CERR << "connecting to " << hub << "/"
                                       << " :" << port << " timed out" << std::endl;
                                  return;
                              }
                              if (ec) {
                                  CERR << "could not establish bulk data connection to " << hub << " :" << port << ": "
                                       << ec.message() << std::endl;
                                  return;
                              }

                              auto &socks = socketList.sockets;
                              if (std::find(socks.begin(), socks.end(), sock) != socks.end()) {
                                  return;
                              }

                              socks.emplace_back(sock);
                              lock.unlock();
                              std::cerr << "." << std::flush;
                              //CERR << "connected to " << addr << ":" << dataPort << ", now have " << m_remoteDataSocket[hubId].sockets.size() << " connections" << std::endl;

                              startThread();
                              serveDirectSocket(sock);
                          });

            lock.lock();
        }

        while (!socketList.connecting.empty() && socketList.sockets.size() < m_numConnections) {
            lock.unlock();
            usleep(10000);
            lock.lock();
        }

        timer.cancel();
        socketList.connecting.clear();

        std::cerr << std::endl;
        if (socketList.sockets.empty()) {
            CERR << "WARNING: could not establish direct data connection to rank " << i << " for hub " << hub << " :"
                 << port << std::endl;
        }
    }
    return true;
}

void DataManager::removeHub(const message::RemoveHub &hub)
{
    closeDirect(hub.id());
}

void DataManager::closeDirect(int hub)
{
    std::unique_lock<std::mutex> lock(m_directSocketsMutex);
    auto it = m_directSockets.find(hub);
    if (it == m_directSockets.end()) {
        return;
    }
    for (auto &socketLists: it->second) {
        for (auto &s: socketLists.connecting) {
            if (s && s->is_open()) {
                boost::system::error_code ec;
                s->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                s->close(ec);
            }
        }
        socketLists.connecting.clear();
        for (auto &s: socketLists.sockets) {
            if (s && s->is_open()) {
                boost::system::error_code ec;
                s->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                s->close(ec);
            }
        }
        socketLists.sockets.clear();
    }
    m_directSockets.erase(it);
}

void DataManager::closeAllDirect()
{
    std::unique_lock<std::mutex> guard(m_directSocketsMutex);
    while (!m_directSockets.empty()) {
        guard.unlock();
        std::set<int> hubs;
        {
            guard.lock();
            for (auto &socks: m_directSockets) {
                hubs.insert(socks.first);
            }
            guard.unlock();
        }
        for (auto hub: hubs)
            closeDirect(hub);
        guard.lock();
    }
}

bool DataManager::dispatch()
{
    bool work = false;
    for (bool gotMsg = true; m_dataSocket->is_open() && gotMsg;) {
        gotMsg = false;
        message::Buffer buf;
        buffer payload;
        {
            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (!m_recvQueue.empty()) {
                gotMsg = true;
                auto &msg = m_recvQueue.front();
                buf = msg.buf;
                payload = std::move(msg.payload);
                m_recvQueue.pop_front();
            }
        }
        if (gotMsg) {
            handle(m_dataSocket, buf, &payload);
            work = true;
            continue;
        }

        if (m_size <= 1) {
            continue;
        }
        auto status = m_req.test();
        if (status && !status->cancelled()) {
            assert(status->tag() == Communicator::TagData);
            m_comm.recv(status->source(), Communicator::TagData, buf.data(), m_msgSize);
            if (buf.payloadSize() > 0) {
                payload.resize(buf.payloadSize());
                m_comm.recv(status->source(), Communicator::TagData, payload.data(), buf.payloadSize());
            }
            work = true;
            gotMsg = true;
            handle(m_dataSocket, buf, &payload);
            m_req = m_comm.irecv(mpi::any_source, Communicator::TagData, &m_msgSize, 1);
        }
    }

    return work;
}

void DataManager::trace(message::Type type)
{
    m_traceMessages = type;
}

bool DataManager::send(std::shared_ptr<asio::ip::tcp::socket> &sock, const message::Message &message,
                       std::shared_ptr<buffer> payload)
{
    if (!sock || !sock->is_open()) {
        CERR << "ERROR: no connection to hub for sending " << message << std::endl;
        return false;
    }

    if (m_asyncSend) {
        //CERR << "async send: " << message << std::endl;
        message::async_send(*sock, message, payload, [this, sock](boost::system::error_code ec) {
            if (ec) {
                CERR << "ERROR: async send to " << sock->remote_endpoint() << " failed: " << ec.message() << std::endl;
            }
        });
        return true;
    }

    return message::send(*sock, message, payload.get());
}

bool DataManager::send(const message::Message &message, std::shared_ptr<buffer> payload)
{
    auto destId = message.destId();
    auto rank = message.destRank();
    if (isLocal(destId)) {
        const int sz = message.size();
        m_comm.send(rank, Communicator::TagData, sz);
        m_comm.send(rank, Communicator::TagData, (const char *)&message, sz);
        if (payload && payload->size() > 0) {
            m_comm.send(rank, Communicator::TagData, payload->data(), payload->size());
        }
        return true;
    }

    std::shared_ptr<boost::asio::ip::tcp::socket> socket = nullptr;
    if (m_useDirectComm) {
        std::unique_lock<std::mutex> lock(m_directSocketsMutex);
        auto it = m_directSockets.find(destId);
        if (it != m_directSockets.end() && it->second.size() > rank) {
#ifdef DEBUG
            CERR << "using direct connection to rank " << rank << " for sending to " << destId << std::endl;
#endif
            socket = it->second[rank].get();
        }
    }
    if (!socket) {
#ifdef DEBUG
        CERR << "falling back to hub relay for sending to " << rank << " of " << destId << std::endl;
#endif
        socket = m_dataSocket;
    }
    return send(socket, message, payload);
}

bool DataManager::requestArray(const std::string &referrer, const std::string &arrayId, int localType, int remoteType,
                               int hub, int rank, const ArrayCompletionHandler &handler)
{
    //CERR << "requesting array: " << arrayId << " for " << referrer << std::endl;
    {
        std::lock_guard<std::mutex> lock(m_requestArrayMutex);
        auto it = m_requestedArrays.find(arrayId);
        if (it != m_requestedArrays.end()) {
            assert(it->second.type == localType);
            it->second.handlers.push_back(handler);
#ifdef DEBUG
            CERR << "requesting array: " << arrayId << " for " << referrer << ", piggybacking..." << std::endl;
#endif
            return true;
        }
#ifdef DEBUG
        CERR << "requesting array: " << arrayId << " for " << referrer << ", requesting..." << std::endl;
#endif
        it = m_requestedArrays.emplace(arrayId, localType).first;
        it->second.handlers.push_back(handler);
    }

    message::RequestObject req(hub, rank, arrayId, remoteType, referrer);
    req.setSenderId(Communicator::the().hubId());
    req.setRank(m_rank);
    send(req);
    return true;
}

bool DataManager::requestObject(const message::AddObject &add, const std::string &objId,
                                const ObjectCompletionHandler &handler)
{
    Object::const_ptr obj = Shm::the().getObjectFromName(objId);
    if (obj) {
#ifdef DEBUG
        std::unique_lock<std::mutex> lock(m_requestObjectMutex);
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << ", already have!" << std::endl;
        lock.unlock();
#endif
        handler(obj);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        m_outstandingAdds[objId].insert(add);

        auto it = m_requestedObjects.find(objId);
        if (it != m_requestedObjects.end()) {
            it->second.completionHandlers.push_back(handler);
#ifdef DEBUG
            CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << ", piggybacking..."
                 << std::endl;
#endif
            return true;
        }
        m_requestedObjects[objId].completionHandlers.push_back(handler);
#ifdef DEBUG
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for " << objId << ", requesting..." << std::endl;
#endif
    }

    message::RequestObject req(add, objId);
    req.setSenderId(Communicator::the().hubId());
    req.setRank(m_rank);
    send(req);
    return true;
}

bool DataManager::requestObject(const std::string &referrer, const std::string &objId, int hub, int rank,
                                const ObjectCompletionHandler &handler)
{
    Object::const_ptr obj = Shm::the().getObjectFromName(objId);
    if (obj) {
#ifdef DEBUG
        std::unique_lock<std::mutex> lock(m_requestObjectMutex);
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for subobj " << objId << ", already have!"
             << std::endl;
        lock.unlock();
#endif
        handler(obj);
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(m_requestObjectMutex);
        auto it = m_requestedObjects.find(objId);
        if (it != m_requestedObjects.end()) {
            it->second.completionHandlers.push_back(handler);
#ifdef DEBUG
            CERR << m_outstandingAdds[objId].size() << " outstanding adds for subobj " << objId << ", piggybacking..."
                 << std::endl;
#endif
            return true;
        }

        m_requestedObjects[objId].completionHandlers.push_back(handler);
#ifdef DEBUG
        CERR << m_outstandingAdds[objId].size() << " outstanding adds for subobj " << objId << ", requesting..."
             << std::endl;
#endif
    }

    message::RequestObject req(hub, rank, objId, referrer);
    req.setSenderId(Communicator::the().hubId());
    req.setRank(m_rank);
    send(req);
    return true;
}

bool DataManager::prepareTransfer(const message::AddObject &add)
{
#ifdef DEBUG
    CERR << "prepareTransfer: retaining " << add.objectName() << std::endl;
#endif
    auto result = m_inTransitObjects.emplace(add);
    if (result.second) {
        result.first->ref();
    }

    updateStatus();

    return true;
}

bool DataManager::completeTransfer(const message::AddObjectCompleted &complete)
{
#ifdef DEBUG
    CERR << "completeTransfer: releasing " << complete.objectName() << std::endl;
#endif

    message::AddObject key(complete);
    auto it = m_inTransitObjects.find(key);
    if (it == m_inTransitObjects.end()) {
        CERR << "AddObject message for completion notification not found: " << complete << ", still "
             << m_inTransitObjects.size() << " objects in transit" << std::endl;
        return true;
    }
    it->takeObject();
    //CERR << "AddObjectCompleted: found request " << *it << " for " << complete << ", still " << m_inTransitObjects.size()-1 << " outstanding" << std::endl;
    m_inTransitObjects.erase(it);

    updateStatus();

    return true;
}

void DataManager::updateStatus()
{
    message::DataTransferState m(m_inTransitObjects.size());
    m.setSenderId(Communicator::the().hubId());
    m.setRank(m_rank);

    if (m_rank == 0)
        Communicator::the().handleMessage(m);
    else
        Communicator::the().forwardToMaster(m);
}

bool DataManager::notifyTransferComplete(const message::AddObject &addObj)
{
    //std::unique_lock<Communicator> guard(Communicator::the());

    //CERR << "sending completion notification for " << objName << std::endl;
    message::AddObjectCompleted complete(addObj);
    complete.setSenderId(Communicator::the().hubId());
    complete.setRank(m_rank);
    int hub = Communicator::the().clusterManager().idToHub(addObj.senderId());
    complete.setDestId(hub);
    complete.setDestRank(addObj.rank());
    //return Communicator::the().clusterManager().sendMessage(hub, complete, addObj.rank());
    return Communicator::the().sendHub(complete);
    //return send(complete);
}

bool DataManager::handle(std::shared_ptr<asio::ip::tcp::socket> &sock, const message::Message &msg, buffer *payload)
{
    //CERR << "handle: " << msg << std::endl;
    using namespace message;

    if (m_traceMessages == message::ANY || msg.type() == m_traceMessages) {
        CERR << "handle: " << msg << std::endl;
    }

    switch (msg.type()) {
    case message::IDENTIFY: {
        auto &mm = static_cast<const Identify &>(msg);
        if (mm.identity() == Identify::REQUEST) {
            if (sock == m_dataSocket) {
                return send(sock, Identify(mm, Identify::LOCALBULKDATA, m_rank));
            }
            return send(sock, Identify(mm, Identify::DIRECTBULKDATA, m_rank));
        }
        return true;
    }
    case message::REQUESTOBJECT:
        return handlePriv(static_cast<const RequestObject &>(msg));
    case message::SENDOBJECT:
        return handlePriv(static_cast<const SendObject &>(msg), payload);
    case message::ADDOBJECTCOMPLETED:
        return handlePriv(msg.as<AddObjectCompleted>());
    default:
        break;
    }

    CERR << "invalid message type " << msg.type() << std::endl;
    return false;
}

class RemoteFetcher: public Fetcher {
public:
    RemoteFetcher(DataManager *dmgr, const message::AddObject &add)
    : m_dmgr(dmgr)
    , m_add(&add)
    , m_hub(Communicator::the().clusterManager().state().getHub(add.senderId()))
    , m_rank(add.rank())
    {}

    RemoteFetcher(DataManager *dmgr, const std::string &referrer, int hub, int rank)
    : m_dmgr(dmgr), m_add(nullptr), m_referrer(referrer), m_hub(hub), m_rank(rank)
    {}

    void requestArray(const std::string &name, int localType, int remoteType,
                      const ArrayCompletionHandler &completeCallback) override
    {
        assert(!m_add);
        m_dmgr->requestArray(m_referrer, name, localType, remoteType, m_hub, m_rank, completeCallback);
    }

    void requestObject(const std::string &name, const ObjectCompletionHandler &completeCallback) override
    {
        m_dmgr->requestObject(m_referrer, name, m_hub, m_rank, completeCallback);
    }

    DataManager *m_dmgr;
    const message::AddObject *m_add;
    const std::string m_referrer;
    int m_hub, m_rank;
};

bool DataManager::handlePriv(const message::RequestObject &req)
{
#ifdef DEBUG
    if (req.isArray()) {
        CERR << "request for array " << req.objectId() << ", type=" << req.arrayType()
             << " (= " << scalarTypeName(req.arrayType()) << ")" << std::endl;
    } else {
        CERR << "request for object " << req.objectId() << std::endl;
    }
#endif

    auto fut = std::async(std::launch::async, [this, req]() {
        setThreadName("dmgr:req:" + std::string(req.objectId()));
        std::shared_ptr<message::SendObject> snd;
        vecostreambuf<buffer> buf;
        buffer &mem = buf.get_vector();
        vistle::oarchive memar(buf);
#ifdef USE_YAS
        memar.setCompressionSettings(Communicator::the().clusterManager().compressionSettings());
#endif
        if (req.isArray()) {
            ArraySaver saver(req.objectId(), req.arrayType(), memar);
            if (!saver.save()) {
                CERR << "failed to serialize array " << req.objectId() << std::endl;
                return false;
            }
            snd.reset(new message::SendObject(req, mem.size()));
        } else {
            Object::const_ptr obj = Shm::the().getObjectFromName(req.objectId());
            if (!obj) {
                CERR << "cannot find object with name " << req.objectId() << std::endl;
                return false;
            }
            obj->saveObject(memar);
            snd.reset(new message::SendObject(req, obj, mem.size()));
        }

        auto compressed = std::make_shared<buffer>();
        *compressed = message::compressPayload(Communicator::the().clusterManager().archiveCompressionMode(), *snd, mem,
                                               Communicator::the().clusterManager().archiveCompressionSpeed());

        snd->setDestId(req.senderId());
        snd->setDestRank(req.rank());
        snd->setSenderId(Communicator::the().hubId());
        snd->setRank(m_rank);
        send(*snd, compressed);
        //CERR << "sent " << snd->payloadSize() << "(" << snd->payloadRawSize() << ") bytes for " << req << " with " << *snd << std::endl;

        return true;
    });

    std::lock_guard<std::mutex> lock(m_sendTaskMutex);
    m_sendTasks.emplace_back(std::move(fut));

    return true;
}

bool DataManager::handlePriv(const message::SendObject &snd, buffer *payload)
{
#ifdef DEBUG
    if (snd.isArray()) {
        CERR << "received array " << snd.objectId() << " for " << snd.referrer() << ", size=" << snd.payloadSize()
             << std::endl;
    } else {
        CERR << "received object " << snd.objectId() << " for " << snd.referrer() << ", size=" << snd.payloadSize()
             << std::endl;
    }
#endif

    auto payload2 = std::make_shared<buffer>(std::move(*payload));
    auto fut = std::async(std::launch::async, [this, snd, payload2]() {
        setThreadName("dmgr:recv:" + std::string(snd.objectId()));
        buffer uncompressed = decompressPayload(snd, *payload2.get());
        vecistreambuf<buffer> membuf(uncompressed);

        if (snd.isArray()) {
            std::unique_lock<std::mutex> lock(m_requestArrayMutex);
            auto it = m_requestedArrays.find(snd.objectId());
            lock.unlock();
            if (it == m_requestedArrays.end()) {
                CERR << "received array " << snd.objectId() << " for " << snd.referrer() << ", but did not find request"
                     << std::endl;
                return false;
            }

            // an array was received
            vistle::iarchive memar(membuf);
            ArrayLoader loader(snd.objectId(), it->second.type, memar);
            if (!loader.load()) {
                CERR << "failed to restore array " << snd.objectId() << std::endl;
                return false;
            }

            lock.lock();
            //CERR << "restored array " << snd.objectId() << ", dangling in memory" << std::endl;
            it = m_requestedArrays.find(snd.objectId());
            if (it == m_requestedArrays.end()) {
                CERR << "restored array " << snd.objectId() << " for " << snd.referrer() << ", but did not find request"
                     << std::endl;
            }
            assert(it != m_requestedArrays.end());
            if (it != m_requestedArrays.end()) {
                auto reqArr = std::move(it->second);
                m_requestedArrays.erase(it);
                lock.unlock();
#ifdef DEBUG
                CERR << "restored array " << snd.objectId() << ", " << reqArr.handlers.size() << " completion handler"
                     << std::endl;
#endif
                for (const auto &completionHandler: reqArr.handlers)
                    completionHandler(snd.objectId());
            }

            return true;
        }

        // an object was received
        std::string objName = snd.objectId();
        auto completionHandler = [this, objName]() mutable -> void {
            std::unique_lock<std::mutex> lock(m_requestObjectMutex);
            auto addIt = m_outstandingAdds.find(objName);
            std::set<message::AddObject> notify;
            if (addIt == m_outstandingAdds.end()) {
                // that's normal if a sub-object was loaded
                //CERR << "no outstanding add for " << objName << std::endl;
            } else {
                notify = std::move(addIt->second);
                m_outstandingAdds.erase(addIt);
            }

            auto objIt = m_requestedObjects.find(objName);
            if (objIt != m_requestedObjects.end()) {
                auto handlers = std::move(objIt->second.completionHandlers);
                auto obj = objIt->second.obj;
                m_requestedObjects.erase(objIt);
                lock.unlock();
                if (!obj) {
                    obj = Shm::the().getObjectFromName(objName);
                    assert(obj);
                }
                for (const auto &handler: handlers) {
                    handler(obj);
                }
            } else {
                CERR << "no outstanding object for " << objName << std::endl;
            }

            for (const auto &add: notify) {
                notifyTransferComplete(add);
            }
        };

        vistle::iarchive memar(membuf);
        memar.setObjectCompletionHandler(completionHandler);
        std::shared_ptr<Fetcher> fetcher(new RemoteFetcher(this, snd.referrer(), snd.senderId(), snd.rank()));
        memar.setFetcher(fetcher);
        Object::const_ptr obj(Object::loadObject(memar));
        if (!obj) {
            CERR << "loading from archive failed for " << objName << std::endl;
        }
        assert(obj);

        {
            std::lock_guard<std::mutex> lock(m_requestObjectMutex);
            auto objIt = m_requestedObjects.find(objName);
            if (objIt == m_requestedObjects.end()) {
                CERR << "object " << objName << " unexpected" << std::endl;
                return false;
            }
        }

        std::lock_guard<std::mutex> lock(m_requestObjectMutex);
        auto objIt = m_requestedObjects.find(objName);
        if (objIt == m_requestedObjects.end()) {
            CERR << "object " << objName << " unexpected" << std::endl;
        } else {
            objIt->second.obj = obj;
        }

        return true;
    });

    std::lock_guard<std::mutex> lock(m_recvTaskMutex);
    m_recvTasks.emplace_back(std::move(fut));

    return true;
}

bool DataManager::handlePriv(const message::AddObjectCompleted &complete)
{
    return completeTransfer(complete);
}

void DataManager::listenLoop()
{
    for (;;) {
        vistle::adaptive_wait(false, this + 1);
        std::lock_guard<std::mutex> guard(m_recvMutex);
        if (m_quit)
            break;
    }
}

void DataManager::recvLoop()
{
    for (;;) {
        bool gotMsg = false;
        if (m_dataSocket->is_open()) {
            message::Buffer buf;
            buffer payload;
            message::error_code ec;
            if (message::recv(*m_dataSocket, buf, ec, false, &payload)) {
                gotMsg = true;
                std::lock_guard<std::mutex> guard(m_recvMutex);
                m_recvQueue.emplace_back(std::move(buf), std::move(payload));
                //CERR << "Data received" << std::endl;
            } else if (ec) {
                CERR << "Data communication error: " << ec.message() << std::endl;
            }

            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }

        vistle::adaptive_wait(gotMsg, this + 2);

        std::lock_guard<std::mutex> guard(m_recvMutex);
        if (m_quit)
            break;
    }
}

void DataManager::sendLoop()
{
    m_ioContext.run();
}

void DataManager::cleanLoop()
{
    auto waitForTasks = [this](std::mutex &mutex, std::deque<std::future<bool>> &tasks, bool workDone,
                               const std::string &kind) -> bool {
        std::unique_lock<std::mutex> lock(mutex);
        while (tasks.size() > 100 || (!workDone && !tasks.empty())) {
            auto front = std::move(tasks.front());
            tasks.pop_front();
            lock.unlock();

            workDone = true;
            bool result = front.get();
            if (!result) {
                CERR << "asynchronous " << kind << " task failed" << std::endl;
            }
            lock.lock();
        }
        return workDone;
    };

    for (;;) {
        bool work = false;
        if (waitForTasks(m_sendTaskMutex, m_sendTasks, work, "send")) {
            work = true;
            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }
        if (waitForTasks(m_recvTaskMutex, m_recvTasks, work, "receive")) {
            work = true;
            std::lock_guard<std::mutex> guard(m_recvMutex);
            if (m_quit)
                break;
        }

        vistle::adaptive_wait(work, this + 3);
        std::lock_guard<std::mutex> guard(m_recvMutex);
        if (m_quit)
            break;
    }
}

DataManager::Msg::Msg(message::Buffer &&buf, buffer &&payload): buf(std::move(buf)), payload(std::move(payload))
{}

} // namespace vistle
