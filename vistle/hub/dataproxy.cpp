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

#if 0
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
        //CERR << "locking socket " << socket.get() << std::endl;
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
        //CERR << "releasing socket " << socket.get() << std::endl;
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

#endif
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

asio::strand &DataProxy::getWriteStrand(boost::shared_ptr<DataProxy::tcp_socket> sock) {
    return getStrand(sock, 0);
}

asio::strand &DataProxy::getReadStrand(boost::shared_ptr<DataProxy::tcp_socket> sock) {
    return getStrand(sock, 1);
}

asio::strand &DataProxy::getStrand(boost::shared_ptr<DataProxy::tcp_socket> sock, int idx) {

    vassert(idx >= 0);
    vassert(idx < 2);
    auto it = m_socketStrand[idx].find(sock.get());
    if (it == m_socketStrand[idx].end()) {
        auto p = std::make_pair(sock.get(), asio::strand(m_io));
        it = m_socketStrand[idx].insert(p).first;
    }
    return it->second;
}

void DataProxy::startThread() {
   boost::lock_guard<boost::mutex> lock(m_mutex);
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

   startAccept();

   message::Identify ident(Identify::REQUEST); // trigger Identify message from remote
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
            {
                 boost::lock_guard<boost::mutex> lock(m_mutex);
                 m_remoteDataSocket[id.senderId()] = sock;
            }
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
            {
                 boost::lock_guard<boost::mutex> lock(m_mutex);
                 m_localDataSocket[id.rank()] = sock;
            }

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
    getReadStrand(sock).dispatch([this, sock, msg](){
        async_recv(*sock, *msg, [this, sock, msg](error_code ec) {
            if (ec) {
                CERR << "recv: error " << ec.message() << std::endl;
                return;
            }
            //CERR << "localMsgRecv: msg received, type=" << msg->type() << std::endl;
            switch(msg->type()) {
            case Message::SENDOBJECT: {
                auto &send = msg->as<const SendObject>();
                size_t payloadSize = send.payloadSize();
                boost::shared_ptr<std::vector<char>> payload(new std::vector<char>(payloadSize));
                getReadStrand(sock).dispatch([this, sock, msg, payload](){
                    boost::asio::async_read(*sock, boost::asio::buffer(*payload), [this, sock, msg, payload](error_code ec, size_t n) {
                        //CERR << "localMsgRecv: received local data, now sending" << std::endl;
                        if (ec) {
                            CERR << "SendObject: error during receive: " << ec.message() << std::endl;
                            return;
                        }
                        if (n != payload->size()) {
                            CERR << "SendObject: only received " << n << " instead of " << payload->size() << " bytes of data from local" << std::endl;
                            return;
                        }
                        auto &send = msg->as<const SendObject>();
                        int hubId = m_hub.idToHub(send.destId());
                        //CERR << "localMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
                        auto remote = getRemoteDataSock(hubId);
                        if (remote) {
                            getWriteStrand(remote).post([remote, send, payload](){
                                message::async_send(*remote, send, [remote](error_code ec){
                                    if (ec) {
                                        CERR << "SendObject: error in remote write: " << ec.message() << std::endl;
                                        return;
                                    }
                                }, payload);
                            });
                        } else {
                            CERR << "did not find remote socket for hub " << hubId << std::endl;
                        }
                        localMsgRecv(sock);
                    });
                });
                break;
            }
            case Message::REQUESTOBJECT: {
                auto &req = msg->as<const RequestObject>();
                int hubId = m_hub.idToHub(req.destId());
                //CERR << "localMsgRecv on " << m_hub.id() << ": sending to " << hubId << std::endl;
                auto remote = getRemoteDataSock(hubId);
                if (remote) {
                    getWriteStrand(remote).post([remote, msg, hubId](){
                        message::async_send(*remote, *msg, [remote, msg, hubId](error_code ec){
                            if (ec) {
                                CERR << "error in forwarding RequestObject msg to remote hub " << hubId << std::endl;
                                return;
                            }
                        });
                    });
                }
                localMsgRecv(sock);
                break;
            }
            case Message::IDENTIFY: {
                auto &ident = msg->as<const Identify>();
                if (ident.identity() != Identify::LOCALBULKDATA) {
                    CERR << "invalid identity " << ident.identity() << " connected to local data port" << std::endl;
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
    });
}

void DataProxy::remoteMsgRecv(boost::shared_ptr<tcp_socket> sock) {

    //CERR << "remoteMsgRecv posted on sock " << sock.get() << std::endl;

    using namespace vistle::message;

    boost::shared_ptr<message::Buffer> msg(new message::Buffer);
    getReadStrand(sock).dispatch([this, sock, msg](){
        async_recv(*sock, *msg, [this, sock, msg](error_code ec){
            if (ec) {
                CERR << "recv: error " << ec.message() << " on sock " << sock.get() << std::endl;
                return;
            }
            //CERR << "remoteMsgRecv success on sock " << sock.get() << ", msg=" << *msg << std::endl;
            switch(msg->type()) {
            case Message::IDENTIFY: {
                auto &ident = msg->as<const Identify>();
                if (ident.identity() == Identify::REQUEST) {
                    Identify ident(Identify::REMOTEBULKDATA, m_hub.id());
                    async_send(*sock, ident, [this, sock](error_code ec){
                        if (ec) {
                            CERR << "send error" << std::endl;
                            return;
                        }
                        remoteMsgRecv(sock);
                    });
                } else if (ident.identity() == Identify::REMOTEBULKDATA) {
                    remoteMsgRecv(sock);
                } else {
                    CERR << "invalid identity " << ident.identity() << " connected to remote data port" << std::endl;
                    remoteMsgRecv(sock);
                }
                break;
            }
            case Message::SENDOBJECT: {
                auto &send = msg->as<const SendObject>();
                //int hubId = m_hub.idToHub(send.senderId());
                //CERR << "remoteMsgRecv on " << m_hub.id() << ": SendObject from " << hubId << std::endl;
                size_t payloadSize = send.payloadSize();
                boost::shared_ptr<std::vector<char>> payload(new std::vector<char>(payloadSize));
                getReadStrand(sock).dispatch([this, sock, msg, payload](){
                    boost::asio::async_read(*sock, boost::asio::buffer(*payload), [this, sock, msg, payload](error_code ec, size_t n) mutable {
                        //CERR << "remoteMsgRecv: received remote data, now sending" << std::endl;
                        if (ec) {
                            CERR << "SendObject: error during receive: " << ec.message() << std::endl;
                            return;
                        }
                        if (n != payload->size()) {
                            CERR << "SendObject: only received " << n << " instead of " << payload->size() << " bytes of data from remote" << std::endl;
                            return;
                        }
                        auto &send = msg->as<const SendObject>();
                        int hubId = m_hub.idToHub(send.destId());
                        vassert(hubId == m_hub.id());
                        auto local = getLocalDataSock(send.destRank());
                        if (local) {
                            getWriteStrand(local).post([local, send, payload](){
                                message::async_send(*local, send, [local](error_code ec){
                                    if (ec) {
                                        CERR << "SendObject: error in local write: " << ec.message() << std::endl;
                                        return;
                                    }
                                }, payload);
                            });
                        } else {
                            CERR << "did not find local socket for hub " << hubId << std::endl;
                        }
                        remoteMsgRecv(sock);
                    });
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
                    getWriteStrand(local).post([this, local, msg, rank, sock](){
                        message::async_send(*local, *msg, [this, msg, rank, sock](error_code ec){
                            if (ec) {
                                CERR << "error in forwarding RequestObject msg to local rank " << rank << std::endl;
                                return;
                            }
                            remoteMsgRecv(sock);
                        });
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
    });
}

bool DataProxy::connectRemoteData(int hubId) {
   boost::lock_guard<boost::mutex> lock(m_mutex);

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

   boost::lock_guard<boost::mutex> lock(m_mutex);
   auto it = m_localDataSocket.find(rank);
   vassert(it != m_localDataSocket.end());
   return it->second;
}

boost::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getRemoteDataSock(int hubId) {

   boost::lock_guard<boost::mutex> lock(m_mutex);
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

}
