#ifndef VISTLE_TUNNEL_H
#define VISTLE_TUNNEL_H

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <core/message.h>

namespace vistle {

class TunnelManager;
class TunnelStream;

//! one tunnel waiting for connections
class Tunnel {

   typedef boost::asio::ip::tcp::socket socket;
   typedef boost::asio::ip::tcp::acceptor acceptor;
   typedef boost::asio::ip::address address;
public:
   Tunnel(TunnelManager &tunnel, unsigned short listenPort,
         address destAddr, unsigned short destPort);
   ~Tunnel();
   const address &destAddr() const;
   unsigned short destPort() const;

private:
   TunnelManager &m_manager;
   address m_destAddr;
   unsigned short m_destPort;

   acceptor m_acceptor;
   boost::shared_ptr<socket> m_listeningSocket;
   std::vector<boost::weak_ptr<TunnelStream>> m_streams;
   void startAccept();
   void handleAccept(const boost::system::error_code &error);
   void handleConnect(boost::shared_ptr<boost::asio::ip::tcp::socket> sock0, boost::shared_ptr<boost::asio::ip::tcp::socket> sock1, const boost::system::error_code &error);
};

//! a single established connection being tunneled
class TunnelStream {

   typedef boost::asio::ip::tcp::socket socket;
 public:
   TunnelStream(boost::shared_ptr<boost::asio::ip::tcp::socket> sock0, boost::shared_ptr<boost::asio::ip::tcp::socket> sock1);
   ~TunnelStream();
   boost::shared_ptr<TunnelStream> self();
   void destroy();

 private:
   boost::shared_ptr<TunnelStream> m_self;
   std::vector<char> m_buf[2];
   std::vector<boost::shared_ptr<socket>> m_sock;
   void handleRead(int sockIdx, boost::system::error_code ec, size_t length);
   void handleWrite(int sockIdx, boost::system::error_code ec);
};

//! manage tunnel creation and destruction
class TunnelManager {

 public:
   typedef boost::asio::ip::tcp::socket socket;
   typedef boost::asio::ip::tcp::acceptor acceptor;
   typedef boost::asio::io_service io_service;

   TunnelManager();
   ~TunnelManager();
   io_service &io();

   bool processRequest(const message::RequestTunnel &msg);

 private:
   bool addTunnel(const message::RequestTunnel &msg);
   bool removeTunnel(const message::RequestTunnel &msg);
   void startThread();
   io_service m_io;
   std::map<unsigned short, boost::shared_ptr<Tunnel>> m_tunnels;
   std::vector<boost::thread> m_threads;
};

}
#endif
   

