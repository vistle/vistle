#include "dataproxy.h"
#include "hub.h"
#include "core/tcpmessage.h"

#define CERR std::cerr << "DataProxy: "

namespace asio = boost::asio;
using boost::system::error_code;

namespace vistle {

using vistle::message::Identify;
using vistle::message::async_send;
using vistle::message::async_recv;

using boost::shared_ptr;

DataProxy::DataProxy(Hub &hub, unsigned short basePort)
: m_hub(hub)
, m_port(basePort)
, m_acceptor(m_io)
{
   CERR << "new" << std::endl;
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
      for (auto &t: m_threads)
         t.join();
      m_threads.clear();
      io().reset();
   }
}

void DataProxy::startThread() {
   if (m_threads.size() < boost::thread::hardware_concurrency()) {
      m_threads.emplace_back(boost::bind(&asio::io_service::run, &m_io));
      CERR << "now " << m_threads.size() << " threads in pool" << std::endl;
   } else {
      CERR << "not starting a new thread, already have " << m_threads.size() << " threads" << std::endl;
   }
}

void DataProxy::startAccept() {

   CERR << "(re-)starting accept" << std::endl;
   boost::shared_ptr<tcp_socket> sock(new tcp_socket(io()));
   m_acceptor.async_accept(*sock, [this, sock](boost::system::error_code ec){handleAccept(ec, sock);});
}

void DataProxy::handleAccept(const boost::system::error_code &error, boost::shared_ptr<tcp_socket> sock) {

   if (error) {
      CERR << "error in accept: " << error.message() << std::endl;
      return;
   }

   CERR << "incoming connection: sock.use_count()=" << sock.use_count() << std::endl;

   startAccept();

   message::Identify ident; // trigger Identify message from remote
   if (message::send(*sock, ident)) {
      CERR << "sent ident msg to remote, sock.use_count()=" << sock.use_count() << std::endl;
   }

   using namespace message;

   boost::shared_ptr<message::Buffer> buf(new message::Buffer);
   message::async_recv(*sock, *buf, [this, sock, buf](const error_code ec){
      if (ec) {
          CERR << "recv error: " << ec.message() << ", sock.use_count()=" << sock.use_count() << std::endl;
          return;
      }
      CERR << "received initial message on incoming connection: type=" << buf->type() << std::endl;
      switch(buf->type()) {
      case message::Message::IDENTIFY: {
         auto &id = buf->as<message::Identify>();
         switch(id.identity()) {
         case Identify::REMOTEBULKDATA: {
            m_remoteDataSocket[id.id()] = sock;
            Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
            async_send(*sock, ident, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                localMsgRecv(sock);
            });
            break;
         }
         case Identify::LOCALBULKDATA: {
            m_localDataSocket[id.id()] = sock;
            Identify ident(Identify::LOCALBULKDATA, -1);
            async_send(*sock, ident, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                remoteMsgRecv(sock);
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

void DataProxy::remoteMsgRecv(boost::shared_ptr<tcp_socket> sock) {

    using namespace vistle::message;

    boost::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg](error_code ec){
        if (ec) {
            CERR << "recv: error " << ec.message() << std::endl;
            return;
        }
        CERR << "remoteMsgRecv: msg received, type=" << msg->type() << std::endl;
        switch(msg->type()) {
        case Message::SENDOBJECT: {
           auto &send = msg->as<const SendObject>();
           int hubId = m_hub.idToHub(send.destId());
           CERR << "handleLocalData on " << m_hub.id() << ": sending to " << hubId << std::endl;
           //sendRemoteData(send, hubId);
           size_t payloadSize = send.payloadSize();
           std::vector<char> payload(payloadSize);
           boost::asio::read(*sock, boost::asio::buffer(payload));
           CERR << "handleLocalData: received local data, now sending" << std::endl;
           //return sendRemoteData(payload.data(), payloadSize, hubId);
           break;
        }
        case Message::REQUESTOBJECT: {
           //auto &req = static_cast<const RequestObject &>(*msg);
           auto &req = msg->as<const RequestObject>();
           int hubId = m_hub.idToHub(req.destId());
           CERR << "remoteMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
           auto remote = getRemoteDataSock(hubId);
           if (remote) {
              message::async_send(*remote, *msg, [this, msg, hubId, remote](error_code ec){
                 if (ec) {
                    CERR << "error in forwarding RequestObject msg to remote hub " << hubId << std::endl;
                    return;
                 }
                 localMsgRecv(remote);
              });
           }
           break;
        }
        default: {
            CERR << "remoteMsgRecv: unsupported msg type " << msg->type() << std::endl;
            break;
        }
        }
    });
}

void DataProxy::localMsgRecv(boost::shared_ptr<tcp_socket> sock) {

    using namespace vistle::message;

    boost::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg](error_code ec){
        if (ec) {
            CERR << "recv: error " << ec.message() << std::endl;
            return;
        }
        switch(msg->type()) {
        case Message::SENDOBJECT: {
           auto &send = msg->as<const SendObject>();
           int hubId = m_hub.idToHub(send.destId());
           CERR << "handleLocalData on " << m_hub.id() << ": sending to " << hubId << std::endl;
           //sendRemoteData(send, hubId);
           size_t payloadSize = send.payloadSize();
           std::vector<char> payload(payloadSize);
           boost::asio::read(*sock, boost::asio::buffer(payload));
           CERR << "handleLocalData: received local data, now sending" << std::endl;
           //return sendRemoteData(payload.data(), payloadSize, hubId);
           break;
        }
        case Message::REQUESTOBJECT: {
           auto &req = msg->as<const RequestObject>();
           int hubId = m_hub.idToHub(req.destId());
           int rank = req.destRank();
           CERR << "handleRemoteData on " << m_hub.id() << ": sending to " << hubId << std::endl;
           if (hubId != m_hub.id()) {
               CERR << "localMsgRecv: hub mismatch" << std::endl;
               return;
           }
           auto local = getLocalDataSock(rank);
           if (local) {
              message::async_send(*local, *msg, [this, msg, rank, local](error_code ec){
                 if (ec) {
                    CERR << "error in forwarding RequestObject msg to local rank " << rank << std::endl;
                    return;
                 }
                 remoteMsgRecv(local);
              });
           }
           break;
        }
        default: {
            CERR << "localMsgRecv: unsupported msg type " << msg->type() << std::endl;
            break;
        }
        }
    });
}

bool DataProxy::connectRemoteData(int hubId) {
   vassert(m_remoteDataSocket.find(hubId) == m_remoteDataSocket.end());

   for (auto &hubData: m_hub.stateTracker().m_hubs) {
      if (hubData.id == hubId) {
         boost::shared_ptr<asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(io()));
         boost::asio::ip::tcp::endpoint dest(hubData.address, hubData.dataPort);

         boost::system::error_code ec;
         asio::connect(*sock, &dest, &dest+1, ec);
         if (ec) {
            CERR << "could not establish bulk data connection to " << hubData.address << ":" << hubData.dataPort << std::endl;
            return false;
         }

         Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
         if (!message::send(*sock, ident)) {
             CERR << "error when establishing bulk data connection to " << hubData.address << ":" << hubData.dataPort << std::endl;
             return false;
         }
         m_remoteDataSocket[hubId] = sock;

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

boost::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getLocalDataSock(int rank) {

   auto it = m_localDataSocket.find(rank);
   vassert(it != m_localDataSocket.end());
   return it->second;
}

boost::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getRemoteDataSock(int hubId) {

   auto it = m_remoteDataSocket.find(hubId);
   if (it == m_remoteDataSocket.end()) {
      if (!connectRemoteData(hubId)) {
         CERR << "failed to establish data connection to remote hub with id " << hubId << std::endl;
         return boost::shared_ptr<asio::ip::tcp::socket>();
      }
   }
   it = m_remoteDataSocket.find(hubId);
   vassert(it != m_remoteDataSocket.end());
   //CERR << "found remote data sock to hub " << hubId << std::endl;
   return it->second;
}

#if 0
bool DataProxy::handleLocalData(const message::Message &recv, shared_ptr<asio::ip::tcp::socket> sock) {
   //CERR << "handleLocalData: " << recv << std::endl;
   using namespace message;
   switch (recv.type()) {
      case Message::REQUESTOBJECT: {
         auto &req = static_cast<const RequestObject &>(recv);
         int hubId = m_hub.idToHub(req.destId());
         CERR << "handleLocalData on " << m_hub.id() << ": sending to " << hubId << std::endl;
         auto remote = getRemoteDataSock(hubId);
         if (remote) {
            message::async_send(*remote, recv, [this, hubId, remote](error_code ec){
                if (ec) {
                    CERR << "error in forwarding RequestObject msg to remote hub " << hubId << std::endl;
                    return;
                }
                message::Buffer buf;
                handleRemoteData(buf, remote);
            });
         }
         break;
      }
      case Message::SENDOBJECT: {
         auto &send = static_cast<const SendObject &>(recv);
         int hubId = m_hub.idToHub(send.destId());
         CERR << "handleLocalData on " << m_hub.id() << ": sending to " << hubId << std::endl;
         //sendRemoteData(send, hubId);
         size_t payloadSize = send.payloadSize();
         std::vector<char> payload(payloadSize);
         boost::asio::read(*sock, boost::asio::buffer(payload));
         CERR << "handleLocalData: received local data, now sending" << std::endl;
         //return sendRemoteData(payload.data(), payloadSize, hubId);
         break;
      }
      default:
         CERR << "invalid message type on local data connection: " << recv.type() << std::endl;
         return false;
   }
   return true;
}

bool DataProxy::handleRemoteData(const message::Message &recv, shared_ptr<asio::ip::tcp::socket> sock) {
   //CERR << "handleRemoteData: " << recv << std::endl;
   using namespace message;
   switch (recv.type()) {
      case Message::IDENTIFY: {
         auto &id = static_cast<const Identify &>(recv);
         if (id.identity() != Identify::REMOTEBULKDATA) {
            CERR << "invalid Identity on remote data connection: " << id.identity() << std::endl;
            return false;
         }
         break;
      }
      case Message::REQUESTOBJECT: {
         auto &req = static_cast<const RequestObject &>(recv);
         //return sendLocalData(req, req.destRank());
         break;
      }
      case Message::SENDOBJECT: {
         auto &send = static_cast<const SendObject &>(recv);
         //sendLocalData(send, send.destRank());
         size_t payloadSize = send.payloadSize();
         std::vector<char> payload(payloadSize);
         boost::asio::read(*sock, boost::asio::buffer(payload));
         //return sendLocalData(payload.data(), payloadSize, send.destRank());
         break;
      }
      default:
         CERR << "invalid message type on remote data connection: " << recv.type() << std::endl;
         return false;
   }
   return true;
}
#endif

}
