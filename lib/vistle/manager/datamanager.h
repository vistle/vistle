#ifndef VISTLE_MANAGER_DATAMANAGER_H
#define VISTLE_MANAGER_DATAMANAGER_H

#include <deque>
#include <functional>
#include <future>
#include <set>
#include <thread>

#include <vistle/core/message.h>
#include <vistle/core/messages.h>
#include <vistle/core/object.h>
#include <vistle/util/buffer.h>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <boost/mpi/request.hpp>
#include <boost/mpi/communicator.hpp>

namespace vistle {

class Communicator;
class StateTracker;
class Object;

class DataManager {
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef boost::asio::ip::address address;
    typedef boost::asio::io_context io_context;

public:
    typedef boost::asio::ip::tcp::socket tcp_socket;

    DataManager(boost::mpi::communicator &comm, unsigned short baseport);
    ~DataManager();
    unsigned short port() const;

    bool handle(std::shared_ptr<tcp_socket> &sock, const message::Message &msg, buffer *payload);
    //! request a remote object for servicing an AddObject request
    bool requestObject(const message::AddObject &add, const std::string &objId, const ObjectCompletionHandler &handler);
    //! request a remote object for resolving a reference to a sub-object
    bool requestObject(const std::string &referrer, const std::string &objId, int hub, int rank,
                       const ObjectCompletionHandler &handler);
    bool requestArray(const std::string &referrer, const std::string &arrayId, int localType, int remoteType, int hub,
                      int rank, const ArrayCompletionHandler &handler);
    bool prepareTransfer(const message::AddObject &add);
    bool completeTransfer(const message::AddObjectCompleted &complete);
    bool notifyTransferComplete(const message::AddObject &add);
    bool connect(boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> &hub);
    bool dispatch();

    void trace(message::Type type);

    bool send(std::shared_ptr<tcp_socket> &sock, const message::Message &message,
              std::shared_ptr<buffer> payload = nullptr);
    bool send(const message::Message &message, std::shared_ptr<buffer> payload = nullptr);

    struct Msg {
        Msg(message::Buffer &&buf, buffer &&payload);
        message::Buffer buf;
        buffer payload;
    };

    bool addHub(const message::AddHub &hub, const message::AddHub::Payload &payload);
    void removeHub(const message::RemoveHub &hub);

private:
    io_context &io();
    void startThread();
    void closeDirect(int hub);
    void closeAllDirect();

    void startAccept(acceptor &a);
    void handleAccept(acceptor &a, const boost::system::error_code &error, std::shared_ptr<tcp_socket> sock);
    void handleConnect(std::shared_ptr<tcp_socket> sock0, std::shared_ptr<tcp_socket> sock1,
                       const boost::system::error_code &error);
    void shutdownSocket(std::shared_ptr<tcp_socket> sock, const std::string &reason);

    bool handlePriv(const message::RequestObject &req);
    bool handlePriv(const message::SendObject &snd, buffer *payload);
    bool handlePriv(const message::AddObjectCompleted &complete);
    bool serveDirectSocket(std::shared_ptr<tcp_socket> sock);
    void updateStatus();

    std::mutex m_recvMutex;
    std::deque<Msg> m_recvQueue;
    bool m_quit = false;

    void recvLoop();
    void sendLoop();
    void listenLoop();
    void cleanLoop();

    boost::mpi::communicator m_comm;
    const int m_rank, m_size;
    boost::mpi::request m_req;
    int m_msgSize;

    unsigned short m_port;
    boost::asio::io_context m_ioContext;
    boost::asio::ip::tcp::acceptor m_acceptorv4;
    boost::asio::ip::tcp::acceptor m_acceptorv6;
    std::shared_ptr<boost::asio::ip::tcp::socket> m_dataSocket;
    std::mutex m_directSocketsMutex;
    struct SocketList {
        SocketList(boost::asio::io_context &io): connectionTimer(io) {}
        SocketList(SocketList &&other): connectionTimer(other.connectionTimer.get_executor())
        {
            std::lock_guard<std::mutex> guard(other.mutex);
            connecting = std::move(other.connecting);
            sockets = std::move(other.sockets);
            current = other.current;
        }
        std::mutex mutex;
        boost::asio::deadline_timer connectionTimer;
        std::set<std::shared_ptr<boost::asio::ip::tcp::socket>> connecting;
        std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> sockets;
        size_t current = 0; //!< round-robin counter for next socket to use
        std::shared_ptr<boost::asio::ip::tcp::socket> get()
        {
            std::lock_guard<std::mutex> guard(mutex);
            if (sockets.empty())
                return nullptr;
            if (current >= sockets.size())
                current = 0;
            return sockets[current++];
        }
    };
    std::map<int, std::vector<SocketList>> m_directSockets; // for every hub and rank, a vector of sockets

    std::set<message::AddObject>
        m_inTransitObjects; //!< objects for which AddObject messages have been sent to remote hubs -- cannot be deleted yet

    std::map<std::string, std::set<message::AddObject>>
        m_outstandingAdds; //!< AddObject messages for which requests to retrieve objects from remote have been sent

    std::mutex m_requestArrayMutex;
    struct RequestedArray {
        RequestedArray(int type): type(type){};
        int type = -1;
        std::vector<ArrayCompletionHandler> handlers;
    };
    std::map<std::string, RequestedArray>
        m_requestedArrays; //!< requests for (sub-)objects which have not been serviced yet

    struct OutstandingObject {
        vistle::Object::const_ptr obj;
        std::vector<ObjectCompletionHandler> completionHandlers;
    };
    std::mutex m_requestObjectMutex;
    std::map<std::string, OutstandingObject>
        m_requestedObjects; //!< requests for (sub-)objects which have not been serviced yet

    std::mutex m_recvTaskMutex;
    std::deque<std::future<bool>> m_recvTasks;

    std::mutex m_sendTaskMutex;
    std::deque<std::future<bool>> m_sendTasks;

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
    std::mutex m_threadsMutex;
    std::vector<std::thread> m_ioThreads;
    std::thread m_listenThread;
    std::thread m_recvThread;
    std::thread m_cleanThread;
    message::Type m_traceMessages = message::INVALID;

    double m_connectionTimeout = 10; // seconds
    size_t m_numConnections = 1; // number of connections per rank (and direction)
    bool m_asyncSend = true; // don't wait for message sends to complete
    bool m_useDirectComm = true; // use direct connections between hubs if possible
};

} // namespace vistle
#endif
