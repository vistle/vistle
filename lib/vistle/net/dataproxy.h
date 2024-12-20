#ifndef VISTLE_DATAPROXY_H
#define VISTLE_DATAPROXY_H

#include <memory>
#include <boost/asio.hpp>
#include <mutex>
#include <thread>
#include <map>
#include <set>
#include <functional>

#include <vistle/core/message.h>
#include <vistle/util/enum.h>

#include "export.h"

namespace vistle {

class StateTracker;

namespace message {
class Identify;
class AddHub;
} // namespace message

class V_NETEXPORT DataProxy {
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef boost::asio::ip::address address;
    typedef boost::asio::io_context io_context;

public:
    typedef boost::asio::ip::tcp::socket tcp_socket;

    DataProxy(StateTracker &state, unsigned short basePort, bool changePort = true);
    ~DataProxy();
    void setHubId(int id);
    void setNumRanks(int size);
    void setBoostArchiveVersion(int ver);
    void setIndexSize(int s);
    void setScalarSize(int s);
    unsigned short port() const;
    void setTrace(message::Type type);

    bool connectRemoteData(const message::AddHub &add);
    bool addSocket(const message::Identify &id, std::shared_ptr<tcp_socket> sock);

private:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(EndPointType, (Local)(Remote))

    int idToHub(int id) const;
    std::recursive_mutex m_mutex;
    int m_hubId;
    int m_numRanks = 0;
    StateTracker &m_stateTracker;
    io_context &io();
    io_context m_io;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
    unsigned short m_port;
    acceptor m_acceptorv4, m_acceptorv6;
    std::vector<std::thread> m_threads;
    std::map<int, std::shared_ptr<tcp_socket>> m_localDataSocket; // MPI rank -> socket
    struct ConnectionData {
        std::vector<std::shared_ptr<tcp_socket>> sockets; // hub id -> socket
        size_t next_socket = 0;
    };
    std::map<int, ConnectionData> m_remoteDataSocket; // hub id -> socket
    int m_boost_archive_version = 0;
    int m_indexSize = 0, m_scalarSize = 0;
    void startAccept(acceptor &a);
    void handleAccept(acceptor &a, const boost::system::error_code &error, std::shared_ptr<tcp_socket> sock);
    void handleConnect(std::shared_ptr<tcp_socket> sock0, std::shared_ptr<tcp_socket> sock1,
                       const boost::system::error_code &error);
    bool serveSocket(const message::Identify &id, std::shared_ptr<tcp_socket> sock);
    void startThread();
    void cleanUp();

    bool answerIdentify(EndPointType type, std::shared_ptr<tcp_socket> sock, const vistle::message::Identify &id);
    bool answerLocalIdentify(std::shared_ptr<tcp_socket> sock, const vistle::message::Identify &id);
    bool answerRemoteIdentify(std::shared_ptr<tcp_socket> sock, const vistle::message::Identify &id);
    std::shared_ptr<tcp_socket> getRemoteDataSock(const message::Message &msg);
    std::shared_ptr<tcp_socket> getLocalDataSock(const message::Message &msg);
    std::shared_ptr<tcp_socket> getDataSock(const message::Message &msg);

    void shutdownSocket(std::shared_ptr<tcp_socket> sock, const std::string &reason);
    bool removeSocket(std::shared_ptr<tcp_socket> sock);

    void msgForward(std::shared_ptr<tcp_socket> sock, EndPointType type);
    void localMsgRecv(std::shared_ptr<tcp_socket> sock);
    void remoteMsgRecv(std::shared_ptr<tcp_socket> sock);

    message::MessageFactory make;
    message::Type m_traceMessages = message::INVALID;

    void printConnections() const;
};

} // namespace vistle

#endif
