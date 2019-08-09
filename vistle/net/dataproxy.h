#ifndef VISTLE_DATAPROXY_H
#define VISTLE_DATAPROXY_H

#include <memory>
#include <boost/asio.hpp>
#include <mutex>
#include <thread>
#include <set>

#include <core/message.h>
#include <util/enum.h>

#include "export.h"

namespace vistle {

class StateTracker;

namespace message {
class Identify;
}

class V_NETEXPORT DataProxy {
   typedef boost::asio::ip::tcp::acceptor acceptor;
   typedef boost::asio::ip::address address;
   typedef boost::asio::io_service io_service;

public:
   typedef boost::asio::ip::tcp::socket tcp_socket;

    DataProxy(StateTracker &state, unsigned short basePort, bool changePort=true);
    ~DataProxy();
    void setHubId(int id);
    void setNumRanks(int size);
    unsigned short port() const;

   bool connectRemoteData(int hubId);

private:
   DEFINE_ENUM_WITH_STRING_CONVERSIONS(EndPointType, (Local)(Remote));

   int idToHub(int id) const;
   io_service &io();
   std::recursive_mutex m_mutex;
   int m_hubId;
   int m_numRanks = 0;
   StateTracker &m_stateTracker;
   unsigned short m_port;
   io_service m_io;
   acceptor m_acceptor;
   std::vector<std::thread> m_threads;
   std::map<int, std::shared_ptr<tcp_socket>> m_localDataSocket; // MPI rank -> socket
   struct RemoteSock {
       RemoteSock(std::shared_ptr<tcp_socket> sock): sock(sock) {}
       bool inUse = false;
       std::shared_ptr<tcp_socket> sock;
   };
   std::map<int, std::vector<RemoteSock>> m_remoteDataSocket; // hub id -> socket
   int m_boost_archive_version;
   void startAccept();
   void handleAccept(const boost::system::error_code &error, std::shared_ptr<tcp_socket> sock);
   void handleConnect(std::shared_ptr<tcp_socket> sock0, std::shared_ptr<tcp_socket> sock1, const boost::system::error_code &error);
   void startThread();
   void cleanUp();

   void answerLocalIdentify(std::shared_ptr<tcp_socket> sock, const vistle::message::Identify &id);
   void answerRemoteIdentify(std::shared_ptr<tcp_socket> sock, const vistle::message::Identify &id);
   std::shared_ptr<tcp_socket> getRemoteDataSock(const message::Message &msg);
   std::shared_ptr<tcp_socket> getLocalDataSock(const message::Message &msg);

   void msgForward(std::shared_ptr<tcp_socket> sock, EndPointType type);
   void localMsgRecv(std::shared_ptr<tcp_socket> sock);
   void remoteMsgRecv(std::shared_ptr<tcp_socket> sock);

   std::set<std::shared_ptr<tcp_socket>> m_connectingSockets;

   message::MessageFactory make;
};

}

#endif
