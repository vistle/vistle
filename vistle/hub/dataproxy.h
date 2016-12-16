#ifndef VISTLE_DATAPROXY_H
#define VISTLE_DATAPROXY_H

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/asio.hpp>
#include <mutex>
#include <thread>

#include <core/message.h>

namespace vistle {

class Hub;

class DataProxy {
   typedef boost::asio::ip::tcp::acceptor acceptor;
   typedef boost::asio::ip::address address;
   typedef boost::asio::io_service io_service;

public:
   typedef boost::asio::ip::tcp::socket tcp_socket;

    DataProxy(Hub &hub, unsigned short basePort);
    ~DataProxy();
    unsigned short port() const;

   bool connectRemoteData(int hubId);

private:
   io_service &io();
   std::recursive_mutex m_mutex;
   Hub &m_hub;
   unsigned short m_port;
   io_service m_io;
   acceptor m_acceptor;
   //boost::shared_ptr<tcp_socket> m_listeningSocket;
   //std::vector<boost::weak_ptr<TunnelStream>> m_streams;
   std::vector<std::thread> m_threads;
   std::map<int, boost::shared_ptr<tcp_socket>> m_localDataSocket; // MPI rank -> socket
   std::map<int, boost::shared_ptr<tcp_socket>> m_remoteDataSocket; // hub id -> socket
   void startAccept();
   void handleAccept(const boost::system::error_code &error, boost::shared_ptr<tcp_socket> sock);
   void handleConnect(boost::shared_ptr<boost::asio::ip::tcp::socket> sock0, boost::shared_ptr<boost::asio::ip::tcp::socket> sock1, const boost::system::error_code &error);
   void startThread();
   void cleanUp();
#if 0
   bool handleLocalData(const message::Message &recv, boost::shared_ptr<tcp_socket> sock);
   bool handleRemoteData(const message::Message &recv, boost::shared_ptr<tcp_socket> sock);
   void handleLocalRead(const message::Message &recv, boost::shared_ptr<tcp_socket> sock);
   void handleRemoteRead(const message::Message &recv, boost::shared_ptr<tcp_socket> sock);
#endif
   boost::shared_ptr<tcp_socket> getRemoteDataSock(int hubId);
   boost::shared_ptr<tcp_socket> getLocalDataSock(int rank);

   void localMsgRecv(boost::shared_ptr<tcp_socket> sock);
   void remoteMsgRecv(boost::shared_ptr<tcp_socket> sock);
};

}

#endif
