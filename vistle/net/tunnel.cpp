#include "tunnel.h"
#include <thread>
#include <boost/system/error_code.hpp>

#include <core/messages.h>
#include <core/assert.h>


namespace asio = boost::asio;
namespace ip = boost::asio::ip;

using std::shared_ptr;

typedef ip::tcp::socket tcp_socket;
typedef ip::tcp::acceptor acceptor;
typedef ip::address_v6 address_v6;

#define CERR std::cerr << "TunnelManager: "

static const size_t BufferSize = 256*1024;

namespace vistle {

using message::RequestTunnel;

TunnelManager::TunnelManager()
{
}

TunnelManager::~TunnelManager()
{
   m_io.stop();
}

void TunnelManager::cleanUp() {

   for (auto &tun: m_tunnels)
      tun.second->cleanUp();

   if (io().stopped()) {
      for (auto &t: m_threads)
         t.join();
      m_threads.clear();
      io().reset();
   }
}

void TunnelManager::startThread() {
   auto &io = m_io;
   m_threads.emplace_back([&io](){ io.run(); });
   CERR << "now " << m_threads.size() << " threads in pool" << std::endl;
}

TunnelManager::io_service &TunnelManager::io() {
   return m_io;
}

bool TunnelManager::processRequest(const message::RequestTunnel &msg) {

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

bool TunnelManager::addTunnel(const message::RequestTunnel &msg) {
   vassert(!msg.remove());
   vassert(msg.destType() != RequestTunnel::Unspecified);
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
      auto endpoints = resolver.resolve({destHost, boost::lexical_cast<std::string>(destPort)});
      destAddr = (*endpoints).endpoint().address();
      std::cerr << destHost << " resolved to " << destAddr << std::endl;
   }
   auto it = m_tunnels.find(listenPort);
   if (it != m_tunnels.end()) {
      CERR << "tunnel listening on port " << listenPort << " already exists" << std::endl;
      return false;
   }
   try {
      std::shared_ptr<Tunnel> tun(new Tunnel(*this, listenPort, destAddr, destPort));
      m_tunnels[listenPort] = tun;
      if (m_threads.size() < std::thread::hardware_concurrency())
         startThread();
   } catch(...) {
      CERR << "error during tunnel creation" << std::endl;
   }
   return true;
}

bool TunnelManager::removeTunnel(const message::RequestTunnel &msg) {
   vassert(msg.remove());
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

   m_tunnels.erase(it);
   return true;
}

#undef CERR
#define CERR std::cerr << "Tunnel: "

Tunnel::Tunnel(TunnelManager &manager, unsigned short listenPort, Tunnel::address destAddr, unsigned short destPort)
: m_manager(manager)
, m_destAddr(destAddr)
, m_destPort(destPort)
, m_acceptor(manager.io())
{
   asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), listenPort);
   m_acceptor.open(endpoint.protocol());
   m_acceptor.set_option(acceptor::reuse_address(true));
   try {
      m_acceptor.bind(endpoint);
   } catch(const boost::system::system_error &err) {
      if (err.code() == boost::system::errc::address_in_use) {
         m_acceptor.close();
         CERR << "failed to listen on port " << listenPort << " - address already in use" << std::endl;
      } else {
         CERR << "listening on port " << listenPort << " failed" << std::endl;
      }
      throw(err);
   }
   m_acceptor.listen();
   CERR << "forwarding connections on port " << listenPort << std::endl;

   startAccept();
}

Tunnel::~Tunnel() {

   for (auto &s: m_streams) {
      if (std::shared_ptr<TunnelStream> p = s.lock())
         p->destroy();
   }
}

void Tunnel::cleanUp() {

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

const Tunnel::address &Tunnel::destAddr() const {
   return m_destAddr;
}

unsigned short Tunnel::destPort() const {
   return m_destPort;
}

void Tunnel::startAccept() {

   m_listeningSocket.reset(new tcp_socket(m_manager.io()));
   m_acceptor.async_accept(*m_listeningSocket, [this](boost::system::error_code ec){handleAccept(ec);});
}

void Tunnel::handleAccept(const boost::system::error_code &error) {

   if (error) {
      CERR << "error in accept: " << error.message() << std::endl;
      return;
   }

   CERR << "incoming connection..." << std::endl;

   std::shared_ptr<tcp_socket> sock(new tcp_socket(m_manager.io()));
   boost::asio::ip::tcp::endpoint dest(m_destAddr, m_destPort);
   CERR << "forwarding to " << dest << std::endl;
   sock->async_connect(dest,
         [this, sock](boost::system::error_code ec){ handleConnect(m_listeningSocket, sock, ec); });

   startAccept();
}

void Tunnel::handleConnect(std::shared_ptr<ip::tcp::socket> sock0, std::shared_ptr<ip::tcp::socket> sock1, const boost::system::error_code &error) {

   if (error) {
      CERR << "error in connect: " << error.message() << std::endl;
      sock0->close();
      sock1->close();
      return;
   }
   CERR << "connected stream..." << std::endl;
   TunnelStream *tun = new TunnelStream(sock0, sock1);
   m_streams.emplace_back(tun->self());
}

#undef CERR
#define CERR std::cerr << "TunnelStream: "

TunnelStream::TunnelStream(std::shared_ptr<boost::asio::ip::tcp::socket> sock0, std::shared_ptr<boost::asio::ip::tcp::socket> sock1)
{
   m_self.reset(this);

   m_sock.push_back(sock0);
   m_sock.push_back(sock1);

   for (size_t i=0; i<m_sock.size(); ++i) {
      m_buf.emplace_back(BufferSize);
   }

   for (size_t i=0; i<m_sock.size(); ++i) {
      m_sock[i]->async_read_some(asio::buffer(m_buf[i].data(), m_buf[i].size()),
            [this, i](boost::system::error_code ec, size_t length) {
               handleRead(self(), i, ec, length);
            });
   }
}

TunnelStream::~TunnelStream() {
   //CERR << "dtor" << std::endl;
   close();
}

std::shared_ptr<TunnelStream> TunnelStream::self() {
   return m_self;
}

void TunnelStream::close() {
   for (auto &sock: m_sock)
      if (sock->is_open())
         sock->close();
}

void TunnelStream::destroy() {

   //CERR << "self destruction" << std::endl;
   close();
   m_self.reset();
}

void TunnelStream::handleRead(std::shared_ptr<TunnelStream> self, size_t sockIdx, boost::system::error_code ec, size_t length) {

   //CERR << "handleRead:  sockIdx=" << sockIdx << ", len=" << length << std::endl;
   int other = (sockIdx+1)%2;
   if (ec) {
      CERR << "read error: " << ec.message() << ", closing stream" << std::endl;
      destroy();
      return;
   }
   async_write(*m_sock[other], asio::buffer(m_buf[sockIdx].data(), length),
         [this, self, sockIdx](boost::system::error_code error, size_t length) {
            handleWrite(self, sockIdx, error);
         });
}

void TunnelStream::handleWrite(std::shared_ptr<TunnelStream> self, size_t sockIdx, boost::system::error_code ec) {

   //CERR << "handleWrite: sockIdx=" << sockIdx << std::endl;
   int other = (sockIdx+1)%2;
   if (ec) {
      CERR << "write error: " << ec.message() << ", closing stream" << std::endl;
      destroy();
      return;
   }
   m_sock[other]->async_read_some(asio::buffer(m_buf[other].data(), m_buf[other].size()),
         [this, self, sockIdx](boost::system::error_code error, size_t length) {
            handleRead(self, sockIdx, error, length);
         });
}

}
