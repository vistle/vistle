#ifndef VISTLE_DATAPROXY_H
#define VISTLE_DATAPROXY_H

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <core/message.h>

namespace vistle {

class Hub;

class DataProxy {
   typedef boost::asio::ip::tcp::socket tcp_socket;
   typedef boost::asio::ip::tcp::acceptor acceptor;
   typedef boost::asio::ip::address address;
   typedef boost::asio::io_service io_service;

public:
    DataProxy(Hub &hub, unsigned short basePort);
    ~DataProxy();
    unsigned short port() const;

   void addLocalData(int rank, boost::shared_ptr<tcp_socket> sock);
   void addRemoteData(int hub, boost::shared_ptr<tcp_socket> sock);

    io_service &io();
private:
   Hub &m_hub;
   unsigned short m_port;
   io_service m_io;
   acceptor m_acceptor;
   //boost::shared_ptr<tcp_socket> m_listeningSocket;
   //std::vector<boost::weak_ptr<TunnelStream>> m_streams;
   std::vector<boost::thread> m_threads;
   std::map<int, boost::shared_ptr<tcp_socket>> m_localDataSocket; // hub id -> socket
   std::map<int, boost::shared_ptr<tcp_socket>> m_remoteDataSocket; // MPI rank -> socket
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
   bool connectRemoteData(int hubId);
   boost::shared_ptr<tcp_socket> getRemoteDataSock(int hubId);
   boost::shared_ptr<tcp_socket> getLocalDataSock(int rank);

   void remoteMsgRecv(boost::shared_ptr<tcp_socket> sock);
   void localMsgRecv(boost::shared_ptr<tcp_socket> sock);
};

}

#endif
