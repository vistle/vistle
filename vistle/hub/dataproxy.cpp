#include "dataproxy.h"
#include "hub.h"
#include <core/tcpmessage.h>
#include <core/message.h>
#include <core/assert.h>
#include <condition_variable>

#define CERR std::cerr << "DataProxy: "

namespace asio = boost::asio;
using boost::system::error_code;
typedef std::lock_guard<std::recursive_mutex> lock_guard;


namespace vistle {

using vistle::message::Identify;
using vistle::message::async_send;
using vistle::message::async_recv;

using std::shared_ptr;

DataProxy::DataProxy(Hub &hub, unsigned short basePort)
: m_hub(hub)
, m_port(basePort)
, m_acceptor(m_io)
, m_boost_archive_version(0)
{
   for (bool connected = false; !connected; ) {
      asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), m_port);
      m_acceptor.open(endpoint.protocol());
      m_acceptor.set_option(acceptor::reuse_address(true));
      try {
         m_acceptor.bind(endpoint);
      } catch(const boost::system::system_error &err) {
         connected = false;
         if (err.code() == boost::system::errc::address_in_use) {
            m_acceptor.close();
            ++m_port;
            continue;
         } else {
            CERR << "listening on port " << m_port << " failed" << std::endl;
            throw(err);
         }
      }
      connected = true;
   }

   CERR << "proxying data connections through port " << m_port << std::endl;
   m_acceptor.listen();

   startAccept();
   startThread();
}

DataProxy::~DataProxy() {
   m_io.stop();
   cleanUp();
}

unsigned short DataProxy::port() const {
    return m_port;
}

asio::io_service &DataProxy::io() {
    return m_io;
}

void DataProxy::cleanUp() {

   if (io().stopped()) {
      m_acceptor.cancel();
      m_acceptor.close();

      boost::system::error_code ec;
      for (auto &s: m_remoteDataSocket)
          s.second->shutdown(tcp_socket::shutdown_both, ec);
      for (auto &s: m_localDataSocket)
          s.second->shutdown(tcp_socket::shutdown_both, ec);
      for (auto &t: m_threads)
         t.join();
      m_threads.clear();
      m_remoteDataSocket.clear();
      m_localDataSocket.clear();
      io().reset();
   }
}

void DataProxy::startThread() {
   lock_guard lock(m_mutex);
   if (true || m_threads.size() < std::thread::hardware_concurrency()) {
   //if (m_threads.size() < 1) {
      auto &io = m_io;
      m_threads.emplace_back([&io](){ io.run(); });
      //CERR << "now " << m_threads.size() << " threads in pool" << std::endl;
   } else {
      CERR << "not starting a new thread, already have " << m_threads.size() << " threads" << std::endl;
   }
}

void DataProxy::startAccept() {

   //CERR << "(re-)starting accept" << std::endl;
   std::shared_ptr<tcp_socket> sock(new tcp_socket(io()));
   m_acceptor.async_accept(*sock, [this, sock](boost::system::error_code ec){handleAccept(ec, sock);});
   startThread();
   startThread();
}

void DataProxy::handleAccept(const boost::system::error_code &error, std::shared_ptr<tcp_socket> sock) {

   if (error) {
      CERR << "error in accept: " << error.message() << std::endl;
      return;
   }

   startAccept();

   message::Identify ident(Identify::REQUEST); // trigger Identify message from remote
   if (message::send(*sock, ident)) {
      //CERR << "sent ident msg to remote, sock.use_count()=" << sock.use_count() << std::endl;
   }

   using namespace message;

   std::shared_ptr<message::Buffer> buf(new message::Buffer);
   message::async_recv(*sock, *buf, [this, sock, buf](const error_code ec, std::shared_ptr<std::vector<char>> payload){
      if (ec) {
          CERR << "recv error after accept: " << ec.message() << ", sock.use_count()=" << sock.use_count() << std::endl;
          return;
      }
      if (payload && payload->size()>0) {
          CERR << "recv error after accept: cannot handle payload of size " << payload->size() << std::endl;
      }
      //CERR << "received initial message on incoming connection: type=" << buf->type() << std::endl;
      switch(buf->type()) {
      case message::IDENTIFY: {
         auto &id = buf->as<message::Identify>();
         switch(id.identity()) {
         case Identify::REMOTEBULKDATA: {
            {
                 lock_guard lock(m_mutex);
                 m_remoteDataSocket[id.senderId()] = sock;

                 if (id.boost_archive_version() != m_boost_archive_version) {
                    std::cerr << "Boost.Archive version on hub " << m_hub.id()  << " is " << m_boost_archive_version << ", but hub " << id.senderId() << " connected with version " << id.boost_archive_version() << std::endl;
                    if (m_boost_archive_version < id.boost_archive_version()) {
                       std::cerr << "Receiving of remote objects from hub " << id.senderId() << " will fail" << std::endl;
                    } else {
                       std::cerr << "Receiving of objects sent to hub " << id.senderId() << " will fail" << std::endl;
                    }
                 }
            }
            Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
            async_send(*sock, ident, nullptr, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                remoteMsgRecv(sock);
            });
            break;
         }
         case Identify::LOCALBULKDATA: {
            {
                 lock_guard lock(m_mutex);
                 m_localDataSocket[id.rank()] = sock;

                 assert(m_boost_archive_version == 0 || m_boost_archive_version == id.boost_archive_version());
                 m_boost_archive_version = id.boost_archive_version();
            }

            Identify ident(Identify::LOCALBULKDATA, -1);
            async_send(*sock, ident, nullptr, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                localMsgRecv(sock);
            });
            break;
         }
         default: {
             CERR << "unexpected identity " << id.identity() << std::endl;
             break;
         }
         }
         break;
      }
      default: {
         CERR << "expected Identify message, got " << buf->type() << std::endl;
         CERR << "got: " << *buf << std::endl;
      }
      }
   });
}

void DataProxy::localMsgRecv(std::shared_ptr<tcp_socket> sock) {

    lock_guard lock(m_mutex);
    using namespace vistle::message;

    std::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg](error_code ec, std::shared_ptr<std::vector<char>> payload){
        if (ec) {
            CERR << "recv: error " << ec.message() << std::endl;
            return;
        }
        //CERR << "localMsgRecv: msg received, type=" << msg->type() << std::endl;
        switch(msg->type()) {
        case SENDOBJECT: {
            auto &send = msg->as<const SendObject>();
            int hubId = m_hub.idToHub(send.destId());
            //CERR << "localMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
            auto remote = getRemoteDataSock(hubId);
            if (remote) {
                async_send(*remote, send, payload, [remote](error_code ec){
                    if (ec) {
                        CERR << "SendObject: error in remote write: " << ec.message() << std::endl;
                        return;
                    }
                });
            } else {
                CERR << "did not find remote socket for hub " << hubId << std::endl;
            }
            break;
        }
        case REQUESTOBJECT: {
            auto &req = msg->as<const RequestObject>();
            int hubId = m_hub.idToHub(req.destId());
            //CERR << "localMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
            auto remote = getRemoteDataSock(hubId);
            if (remote) {
                async_send(*remote, *msg, nullptr, [remote, msg, hubId](error_code ec){
                    if (ec) {
                        CERR << "error in forwarding RequestObject msg to remote hub " << hubId << std::endl;
                        return;
                    }
                });
            }
            break;
        }
        case IDENTIFY: {
            auto &ident = msg->as<const Identify>();
            if (ident.identity() != Identify::LOCALBULKDATA) {
                CERR << "invalid identity " << ident.identity() << " connected to local data port" << std::endl;
            }
            break;
        }
        default: {
            CERR << "localMsgRecv: unsupported msg type " << msg->type() << std::endl;
            break;
        }
        }
        localMsgRecv(sock);
    });
}

void DataProxy::remoteMsgRecv(std::shared_ptr<tcp_socket> sock) {

    //CERR << "remoteMsgRecv posted on sock " << sock.get() << std::endl;
    lock_guard lock(m_mutex);

    using namespace vistle::message;

    std::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg](error_code ec, std::shared_ptr<std::vector<char>> payload){
        if (ec) {
            CERR << "recv: error " << ec.message() << " on sock " << sock.get() << std::endl;
            return;
        }
        //CERR << "remoteMsgRecv success on sock " << sock.get() << ", msg=" << *msg << std::endl;
        switch(msg->type()) {
        case IDENTIFY: {
            auto &ident = msg->as<const Identify>();
            if (ident.identity() == Identify::REQUEST) {
                Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
                async_send(*sock, ident, nullptr, [this, sock](error_code ec){
                    if (ec) {
                        CERR << "send error" << std::endl;
                        return;
                    }
                });
            } else if (ident.identity() == Identify::REMOTEBULKDATA) {
                if (ident.boost_archive_version() != m_boost_archive_version) {
                    std::cerr << "Boost.Archive version on hub " << m_hub.id()  << " is " << m_boost_archive_version << ", but hub " << ident.senderId() << " connected with version " << ident.boost_archive_version() << std::endl;
                    if (m_boost_archive_version < ident.boost_archive_version()) {
                        std::cerr << "Receiving of remote objects from hub " << ident.senderId() << " will fail" << std::endl;
                    } else {
                        std::cerr << "Receiving of objects sent to hub " << ident.senderId() << " will fail" << std::endl;
                    }
                 }
            } else {
                CERR << "invalid identity " << ident.identity() << " connected to remote data port" << std::endl;
            }
            break;
        }
        case SENDOBJECT: {
            auto &send = msg->as<const SendObject>();
            //int hubId = m_hub.idToHub(send.senderId());
            //CERR << "remoteMsgRecv on " << m_hub.id() << ": SendObject from " << hubId << std::endl;
            int hubId = m_hub.idToHub(send.destId());
            vassert(hubId == m_hub.id());
            auto local = getLocalDataSock(send.destRank());
            if (local) {
                async_send(*local, send, payload, [this, local](error_code ec){
                    if (ec) {
                        CERR << "SendObject: error in local write: " << ec.message() << std::endl;
                        return;
                    }
                });
            } else {
                CERR << "did not find local socket for hub " << hubId << std::endl;
            }
            break;
        }
        case REQUESTOBJECT: {
            auto &req = msg->as<const RequestObject>();
            int hubId = m_hub.idToHub(req.destId());
            int rank = req.destRank();
            if (hubId != m_hub.id()) {
                CERR << "remoteMsgRecv: hub mismatch" << std::endl;
                CERR << "remoteMsgRecv: on " << m_hub.id() << ": sending to " << hubId << std::endl;
            } else if (auto local = getLocalDataSock(rank)) {
                async_send(*local, *msg, nullptr, [this, msg, rank, sock](error_code ec){
                    if (ec) {
                        CERR << "error in forwarding RequestObject msg to local rank " << rank << std::endl;
                        return;
                    }
                });
            }
            break;
        }
        default: {
            CERR << "remoteMsgRecv: unsupported msg type " << msg->type() << std::endl;
            break;
        }
        }
        remoteMsgRecv(sock);
    });
}

bool DataProxy::connectRemoteData(int hubId) {
   lock_guard lock(m_mutex);

   vassert(m_remoteDataSocket.find(hubId) == m_remoteDataSocket.end());

   for (auto &hubData: m_hub.stateTracker().m_hubs) {
      if (hubData.id == hubId) {
         std::shared_ptr<asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(io()));
         boost::asio::ip::tcp::endpoint dest(hubData.address, hubData.dataPort);

         boost::system::error_code ec;
         asio::connect(*sock, &dest, &dest+1, ec);
         if (ec) {
            CERR << "could not establish bulk data connection to " << hubData.address << ":" << hubData.dataPort << std::endl;
            return false;
         }
         m_remoteDataSocket[hubId] = sock;
         remoteMsgRecv(sock);

#if 0
         Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
         if (!message::send(*sock, ident)) {
             CERR << "error when establishing bulk data connection to " << hubData.address << ":" << hubData.dataPort << std::endl;
             return false;
         }
#endif

#if 0
         addSocket(sock, message::Identify::REMOTEBULKDATA);
         sendMessage(sock, message::Identify(message::Identify::REMOTEBULKDATA, m_hubId));

         addRemoteData(hubId, sock);
         addClient(sock);
#endif

         CERR << "connected to hub (data) at " << hubData.address << ":" << hubData.dataPort << std::endl;
         return true;
      }
   }

   CERR << "don't know hub " << hubId << std::endl;
   return false;
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getLocalDataSock(int rank) {

   lock_guard lock(m_mutex);
   auto it = m_localDataSocket.find(rank);
   vassert(it != m_localDataSocket.end());
   return it->second;
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getRemoteDataSock(int hubId) {

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
   vassert(it != m_remoteDataSocket.end());
   //CERR << "found remote data sock to hub " << hubId << std::endl;
   return it->second;
}

}
