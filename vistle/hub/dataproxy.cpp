#include "dataproxy.h"
#include "hub.h"
#include <core/tcpmessage.h>
#include <core/message.h>
#include <condition_variable>

#define CERR std::cerr << "DataProxy: "

namespace asio = boost::asio;
using boost::system::error_code;
typedef boost::lock_guard<boost::recursive_mutex> lock_guard;


namespace vistle {

using vistle::message::Identify;
using vistle::message::async_send;
using vistle::message::async_recv;

using boost::shared_ptr;

struct Deleter {
    DataProxy::tcp_socket *sock;
    Deleter(DataProxy::tcp_socket *sock): sock(sock) {
        CERR << "acquiring mutex for " << sock << std::endl;
    }
    void operator()(lock_guard *p) {
        delete p;
        CERR << "released mutex for " << sock << std::endl;
    }
};

//boost::shared_ptr<lock_guard> locker(new lock_guard(m_socketMutex[sock.get()]), Deleter(sock.get()));

#define LOCKER(sock) boost::shared_ptr<lock_guard> _LOCKER_(new lock_guard(m_socketMutex[sock.get()]), Deleter(sock.get()))

template<class SocketType>
struct SocketGuardCore
{
    typedef SocketType Socket;

    static std::mutex s_mutex;
    static std::map<Socket*, std::condition_variable> s_conditions;
    static std::map<Socket*, std::mutex> s_mutexes;
    static std::map<Socket*, bool> s_busy;

    boost::shared_ptr<Socket> socket;
    std::mutex *mutex;
    std::condition_variable *condition;
    bool *busy;
    boost::shared_ptr<std::unique_lock<std::mutex>> lock;

    SocketGuardCore(boost::shared_ptr<Socket> socket)
        : socket(socket)
        , mutex(nullptr)
        , condition(nullptr)
        , busy(nullptr)
    {
        CERR << "locking socket " << socket.get() << std::endl;
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            mutex = &s_mutexes[socket.get()];
            condition = &s_conditions[socket.get()];
            auto it = s_busy.find(socket.get());
            if (it == s_busy.end()) {
                it = s_busy.emplace(socket.get(), false).first;
            }
            busy = &it->second;
            vassert(mutex);
            vassert(condition);
            vassert(busy);
        }
        lock.reset(new std::unique_lock<std::mutex>(*mutex));
        while(*busy) {
            condition->wait(*lock);
        }
        *busy = true;
    }

    ~SocketGuardCore() {
        CERR << "releasing socket " << socket.get() << std::endl;
        *busy = false;
        lock.reset();
        condition->notify_all();
    }

};

typedef SocketGuardCore<DataProxy::tcp_socket> SocketGuard;
typedef boost::shared_ptr<SocketGuard> SocketGuardPtr;

template<class Socket>
std::mutex SocketGuardCore<Socket>::s_mutex;
template<class Socket>
std::map<Socket*, std::condition_variable> SocketGuardCore<Socket>::s_conditions;
template<class Socket>
std::map<Socket*, std::mutex> SocketGuardCore<Socket>::s_mutexes;
template<class Socket>
std::map<Socket*, bool> SocketGuardCore<Socket>::s_busy;

DataProxy::DataProxy(Hub &hub, unsigned short basePort)
: m_hub(hub)
, m_port(basePort)
, m_acceptor(m_io)
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
   startThread();
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
            m_remoteDataSocket[id.senderId()] = sock;
            Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
            async_send(*sock, ident, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                remoteMsgRecv(sock);
            });
            break;
         }
         case Identify::LOCALBULKDATA: {
            m_localDataSocket[id.rank()] = sock;
            m_socketMutex[sock.get()];

            Identify ident(Identify::LOCALBULKDATA, -1);
            async_send(*sock, ident, [this, sock](error_code ec){
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

void DataProxy::localMsgRecv(boost::shared_ptr<tcp_socket> sock) {

    using namespace vistle::message;

    boost::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg](error_code ec) mutable {
        if (ec) {
            CERR << "recv: error " << ec.message() << std::endl;
            return;
        }
        CERR << "localMsgRecv: msg received, type=" << msg->type() << std::endl;
        switch(msg->type()) {
        case Message::SENDOBJECT: {
           auto &send = msg->as<const SendObject>();
           size_t payloadSize = send.payloadSize();
           boost::shared_ptr<std::vector<char>> payload(new std::vector<char>(payloadSize));
           boost::asio::async_read(*sock, boost::asio::buffer(*payload), [this, sock, msg, payload](error_code ec, size_t n) mutable {
               CERR << "localMsgRecv: received local data, now sending" << std::endl;
               if (ec) {
                   CERR << "SendObject: error during receive: " << ec.message() << std::endl;
                   return;
               }
               if (n != payload->size()) {
                   CERR << "SendObject: only received " << n << " instead of " << payload->size() << " bytes of data from local" << std::endl;
                   return;
               }
               uint32_t sz = htonl(msg->size());
               std::vector<boost::asio::mutable_buffer> buffers;
               buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
               buffers.push_back(boost::asio::buffer(msg->data(), msg->size()));
               buffers.push_back(boost::asio::buffer(*payload));
               auto &send = msg->as<const SendObject>();
               int hubId = m_hub.idToHub(send.destId());
               CERR << "localMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
               auto remote = getRemoteDataSock(hubId);
               if (remote) {
                  SocketGuardPtr locker(new SocketGuard(remote));
                  boost::asio::async_write(*remote, buffers, [remote, locker](error_code ec, size_t n){
                      if (ec) {
                          CERR << "SendObject: error in remote write: " << ec.message() << std::endl;
                          return;
                      }
                      CERR << "SendObject: complete, wrote " << n << " bytes" << std::endl;
                  });
               } else {
                   CERR << "did not find remote socket for hub " << hubId << std::endl;
               }
               localMsgRecv(sock);
           });
           break;
        }
        case Message::REQUESTOBJECT: {
           //auto &req = static_cast<const RequestObject &>(*msg);
           auto &req = msg->as<const RequestObject>();
           int hubId = m_hub.idToHub(req.destId());
           CERR << "localMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
           auto remote = getRemoteDataSock(hubId);
           if (remote) {
              SocketGuardPtr locker(new SocketGuard(remote));
              message::async_send(*remote, *msg, [remote, locker, msg, hubId](error_code ec){
                 if (ec) {
                    CERR << "error in forwarding RequestObject msg to remote hub " << hubId << std::endl;
                    return;
                 }
              });
           }
           localMsgRecv(sock);
           break;
        }
        case Message::IDENTIFY: {
           auto &ident = msg->as<const Identify>();
           if (ident.identity() != Identify::REMOTEBULKDATA) {
              CERR << "invalid identity " << ident.identity() << " connected to remote data port" << std::endl;
           }
           localMsgRecv(sock);
           break;
        }
        default: {
            CERR << "localMsgRecv: unsupported msg type " << msg->type() << std::endl;
            localMsgRecv(sock);
            break;
        }
        }
    });
}

void DataProxy::remoteMsgRecv(boost::shared_ptr<tcp_socket> sock) {

    CERR << "remoteMsgRecv posted on sock " << sock.get() << std::endl;

    using namespace vistle::message;

    boost::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg](error_code ec){
        if (ec) {
            CERR << "recv: error " << ec.message() << " on sock " << sock.get() << std::endl;
            return;
        }
        CERR << "remoteMsgRecv success on sock " << sock.get() << ", msg=" << *msg << std::endl;
        switch(msg->type()) {
        case Message::IDENTIFY: {
            auto &ident = msg->as<const Identify>();
            if (ident.identity() == Identify::UNKNOWN) {
               Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
               SocketGuardPtr locker(new SocketGuard(sock));
               async_send(*sock, ident, [this, sock, locker](error_code ec){
                  if (ec) {
                     CERR << "send error" << std::endl;
                     return;
                  }
                  remoteMsgRecv(sock);
               });
            } else if (ident.identity() == Identify::REMOTEBULKDATA) {
                remoteMsgRecv(sock);
            } else {
                remoteMsgRecv(sock);
            }
            break;
        }
        case Message::SENDOBJECT: {
           auto &send = msg->as<const SendObject>();
           int hubId = m_hub.idToHub(send.senderId());
           CERR << "remoteMsgRecv on " << m_hub.id() << ": SendObject from " << hubId << std::endl;
           size_t payloadSize = send.payloadSize();
           boost::shared_ptr<std::vector<char>> payload(new std::vector<char>(payloadSize));
           boost::asio::async_read(*sock, boost::asio::buffer(*payload), [this, sock, msg, payload](error_code ec, size_t n) mutable {
               CERR << "remoteMsgRecv: received remote data, now sending" << std::endl;
               if (ec) {
                   CERR << "SendObject: error during receive: " << ec.message() << std::endl;
                   return;
               }
               if (n != payload->size()) {
                   CERR << "SendObject: only received " << n << " instead of " << payload->size() << " bytes of data from remote" << std::endl;
                   return;
               }
               uint32_t sz = htonl(msg->size());
               std::vector<boost::asio::mutable_buffer> buffers;
               buffers.push_back(boost::asio::buffer(&sz, sizeof(sz)));
               buffers.push_back(boost::asio::buffer(msg->data(), msg->size()));
               buffers.push_back(boost::asio::buffer(*payload));
               auto &send = msg->as<const SendObject>();
               int hubId = m_hub.idToHub(send.destId());
               vassert(hubId == m_hub.id());
               auto local = getLocalDataSock(send.destRank());
               if (local) {
                  SocketGuardPtr locker(new SocketGuard(local));
                  boost::asio::async_write(*local, buffers, [local, locker](error_code ec, size_t n){
                      if (ec) {
                          CERR << "SendObject: error in local write: " << ec.message() << std::endl;
                          return;
                      }
                      CERR << "SendObject: to local complete, wrote " << n << " bytes" << std::endl;
                  });
               } else {
                   CERR << "did not find local socket for hub " << hubId << std::endl;
               }
               remoteMsgRecv(sock);
           });
           break;
        }
        case Message::REQUESTOBJECT: {
           auto &req = msg->as<const RequestObject>();
           int hubId = m_hub.idToHub(req.destId());
           int rank = req.destRank();
           if (hubId != m_hub.id()) {
               CERR << "remoteMsgRecv: hub mismatch" << std::endl;
               CERR << "remoteMsgRecv: on " << m_hub.id() << ": sending to " << hubId << std::endl;
               remoteMsgRecv(sock);
               return;
           }
           auto local = getLocalDataSock(rank);
           if (local) {
              SocketGuard locker(local);
              message::async_send(*local, *msg, [this, msg, rank, sock, locker](error_code ec){
                 if (ec) {
                    CERR << "error in forwarding RequestObject msg to local rank " << rank << std::endl;
                    return;
                 }
                 remoteMsgRecv(sock);
              });
           }
           break;
        }
        default: {
            CERR << "remoteMsgRecv: unsupported msg type " << msg->type() << std::endl;
            remoteMsgRecv(sock);
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
         m_remoteDataSocket[hubId] = sock;
         m_socketMutex[sock.get()];
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

boost::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getLocalDataSock(int rank) {

   auto it = m_localDataSocket.find(rank);
   vassert(it != m_localDataSocket.end());
   return it->second;
}

boost::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getRemoteDataSock(int hubId) {

   auto it = m_remoteDataSocket.find(hubId);
#if 0
   if (it == m_remoteDataSocket.end()) {
      if (!connectRemoteData(hubId)) {
         CERR << "failed to establish data connection to remote hub with id " << hubId << std::endl;
         return boost::shared_ptr<asio::ip::tcp::socket>();
      }
   }
   it = m_remoteDataSocket.find(hubId);
#endif
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
