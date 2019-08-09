#include "dataproxy.h"
#include <core/tcpmessage.h>
#include <core/message.h>
#include <core/assert.h>
#include <core/statetracker.h>
#include <condition_variable>
#include <boost/asio/deadline_timer.hpp>

#define CERR std::cerr << "DataProxy: "

namespace asio = boost::asio;
using boost::system::error_code;
typedef std::unique_lock<std::recursive_mutex> lock_guard;


namespace vistle {

using vistle::message::Identify;
using vistle::message::async_send;
using vistle::message::async_recv;

using std::shared_ptr;

DataProxy::DataProxy(StateTracker &state, unsigned short basePort, bool changePort)
: m_hubId(message::Id::Invalid)
, m_stateTracker(state)
, m_port(basePort)
, m_acceptor(m_io)
, m_boost_archive_version(0)
{
   for (bool connected = false; !connected; ) {
      asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), m_port);
      try {
         m_acceptor.open(endpoint.protocol());
      } catch (const boost::system::system_error &err) {
         if (err.code() == boost::system::errc::address_family_not_supported) {
            endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port);
            m_acceptor.open(endpoint.protocol());
         } else {
            throw(err);
         }
      }
      m_acceptor.set_option(acceptor::reuse_address(true));
      try {
         m_acceptor.bind(endpoint);
      } catch(const boost::system::system_error &err) {
         connected = false;
         if (err.code() == boost::system::errc::address_in_use) {
            m_acceptor.close();
            if (changePort) {
                ++m_port;
                continue;
            }
         } else {
            CERR << "listening on port " << m_port << " failed" << std::endl;
            throw(err);
         }
      }
      connected = true;
   }

   CERR << "proxying data connections through port " << m_port << std::endl;
   m_acceptor.listen();
}

DataProxy::~DataProxy() {
   m_io.stop();
   cleanUp();
}

void DataProxy::setHubId(int id) {
   m_hubId = id;
   make.setId(m_hubId);
   make.setRank(0);

   startAccept();
   startThread();
}

void DataProxy::setNumRanks(int size) {
    m_numRanks = size;
}

unsigned short DataProxy::port() const {
    return m_port;
}

asio::io_service &DataProxy::io() {
    return m_io;
}

int DataProxy::idToHub(int id) const {

   if (id >= message::Id::ModuleBase)
      return m_stateTracker.getHub(id);

   if (id == message::Id::LocalManager || id == message::Id::LocalHub)
       return m_hubId;

   return m_stateTracker.getHub(id);
}

void DataProxy::cleanUp() {

   if (io().stopped()) {
      m_acceptor.cancel();
      m_acceptor.close();

      boost::system::error_code ec;
      for (auto &ssv: m_remoteDataSocket) {
          for (auto &ss: ssv.second) {
              ss.sock->shutdown(tcp_socket::shutdown_both, ec);
          }
      }
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

void DataProxy::answerLocalIdentify(std::shared_ptr<DataProxy::tcp_socket> sock, const message::Identify &ident) {
    if (ident.identity() != Identify::LOCALBULKDATA) {
        CERR << "invalid identity " << ident.identity() << " connected to local data port" << std::endl;
    }
}

void DataProxy::answerRemoteIdentify(std::shared_ptr<DataProxy::tcp_socket> sock, const message::Identify &ident) {
    if (ident.identity() == Identify::REQUEST) {
        auto ident = make.message<Identify>(Identify::REMOTEBULKDATA, m_hubId);
        async_send(*sock, ident, nullptr, [sock](error_code ec){
            if (ec) {
                CERR << "send error" << std::endl;
                return;
            }
        });
    } else if (ident.identity() == Identify::REMOTEBULKDATA) {
        if (ident.boost_archive_version() != m_boost_archive_version) {
            std::cerr << "Boost.Archive version on hub " << m_hubId  << " is " << m_boost_archive_version << ", but hub " << ident.senderId() << " connected with version " << ident.boost_archive_version() << std::endl;
            if (m_boost_archive_version < ident.boost_archive_version()) {
                std::cerr << "Receiving of remote objects from hub " << ident.senderId() << " will fail" << std::endl;
            } else {
                std::cerr << "Receiving of objects sent to hub " << ident.senderId() << " will fail" << std::endl;
            }
        }
    } else {
        CERR << "invalid identity " << ident.identity() << " connected to remote data port" << std::endl;
    }
}

void DataProxy::startThread() {
   lock_guard lock(m_mutex);
   if (true || m_threads.size() < std::thread::hardware_concurrency()) {
   //if (m_threads.size() < 1) {
      auto &io = m_io;
      m_threads.emplace_back([&io](){
          io.run();
      });
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

   auto ident = make.message<Identify>(Identify::REQUEST);
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
                 m_remoteDataSocket[id.senderId()].emplace_back(sock);
             }

             if (id.boost_archive_version() != m_boost_archive_version) {
                 std::cerr << "Boost.Archive version on hub " << m_hubId  << " is " << m_boost_archive_version << ", but hub " << id.senderId() << " connected with version " << id.boost_archive_version() << std::endl;
                 if (m_boost_archive_version < id.boost_archive_version()) {
                     std::cerr << "Receiving of remote objects from hub " << id.senderId() << " will fail" << std::endl;
                 } else {
                     std::cerr << "Receiving of objects sent to hub " << id.senderId() << " will fail" << std::endl;
                 }
             }
            auto ident = make.message<Identify>(Identify::REMOTEBULKDATA, m_hubId);
            async_send(*sock, ident, nullptr, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                msgForward(sock, Local);
            });
            break;
         }
         case Identify::LOCALBULKDATA: {
            {
                 lock_guard lock(m_mutex);
                 m_localDataSocket[id.rank()] = sock;
            }

            assert(m_boost_archive_version == 0 || m_boost_archive_version == id.boost_archive_version());
            m_boost_archive_version = id.boost_archive_version();

            auto ident = make.message<Identify>(Identify::LOCALBULKDATA, -1);
            async_send(*sock, ident, nullptr, [this, sock](error_code ec){
                if (ec) {
                    CERR << "send error" << std::endl;
                    return;
                }
                msgForward(sock, Remote);
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

void DataProxy::msgForward(std::shared_ptr<tcp_socket> sock, EndPointType type) {

    using namespace vistle::message;

    std::shared_ptr<message::Buffer> msg(new message::Buffer);
    async_recv(*sock, *msg, [this, sock, msg, type](error_code ec, std::shared_ptr<std::vector<char>> payload){
        if (ec) {
            CERR << "msgForward, dest=" << toString(type) << ": error " << ec.message() << std::endl;
            return;
        }

        msgForward(sock, type);

        bool needPayload = false;
        bool forward = false;
        switch(msg->type()) {
        case IDENTIFY: {
            auto &ident = msg->as<const Identify>();
            type==Local ? answerRemoteIdentify(sock, ident) : answerLocalIdentify(sock, ident);
            break;
        }
        case SENDOBJECT: {
            forward = true;
            needPayload = true;
#ifdef DEBUG
            auto &send = msg->as<const SendObject>();
            if (send.isArray()) {
                CERR << "local SendObject: array " << send.objectId() << ", size=" << send.payloadSize() << std::endl;
            } else {
                CERR << "local SendObject: object " << send.objectId() << ", size=" << send.payloadSize() << std::endl;
            }
#endif
            break;
        }
        case REQUESTOBJECT: {
            forward = true;
            break;
        }
        case ADDOBJECTCOMPLETED: {
            forward = true;
            break;
        }
        default: {
            CERR << "have unexpected message " << *msg << std::endl;
            forward = true;
            break;
        }
        }

        if (!needPayload) {
            if (payload) {
                CERR << "have unnecessary payload for " << *msg << std::endl;
            }
            needPayload = true;
        }

        if (forward) {
            auto dest = type==Local ? getLocalDataSock(*msg) : getRemoteDataSock(*msg);
            if (dest) {
                async_send(*dest, *msg, payload, [type, dest, payload](error_code ec) mutable {
                    message::return_buffer(payload);
                    if (ec) {
                        CERR << "error in write to " << toString(type) << ": " << ec.message() << std::endl;
                        return;
                    }
                });
            } else {
                CERR << "no destination socket for " << toString(type) << " forward of " << *msg << std::endl;
            }
        } else {
            message::return_buffer(payload);
        }
    });
}

bool DataProxy::connectRemoteData(int hubId) {
   lock_guard lock(m_mutex);

   const auto &remote = m_stateTracker.getHubData(hubId);

   if (remote.id == message::Id::Invalid) {
       CERR << "don't know hub with id " << hubId << std::endl;
       return false;
   }

   asio::deadline_timer timer(io());
   timer.expires_from_now(boost::posix_time::seconds(10));
   timer.async_wait([this](const boost::system::error_code &ec){
       if (ec == asio::error::operation_aborted) {
           // timer was cancelled
           return;
       }
       if (ec) {
           CERR << "timer failed: " << ec << std::endl;
           lock_guard lock(m_mutex);
           m_connectingSockets.clear();
           return;
       }

       lock_guard lock(m_mutex);
       for (auto &s: m_connectingSockets) {
           s->cancel();
           s->close();
       }
       m_connectingSockets.clear();
   });

   size_t numconn = std::min(std::max(1, std::max(m_numRanks, remote.numRanks)), 12);
   size_t numtries = numconn - m_remoteDataSocket[hubId].size();
   CERR << "establishing data connecton to hub at " << remote.address << ":" << remote.dataPort << " with " << numconn << " parallel connections, " << numtries << " tries" << std::endl;
   boost::asio::ip::tcp::endpoint dest(remote.address, remote.dataPort);

   size_t count=0;
   while (m_remoteDataSocket[hubId].size() < numconn && count < numtries) {
       ++count;
       auto sock = std::make_shared<asio::ip::tcp::socket>(io());
       m_connectingSockets.insert(sock);
       lock.unlock();

       sock->async_connect(dest, [this, sock, remote, hubId](const boost::system::error_code &ec){
           lock_guard lock(m_mutex);
           m_connectingSockets.erase(sock);

           if (ec == asio::error::operation_aborted) {
               return;
           }
           if (ec == asio::error::timed_out) {
               return;
           }
           if (ec) {
               CERR << "could not establish bulk data connection to " << remote.address << ":" << remote.dataPort << ": " << ec << std::endl;
               return;
           }

           m_remoteDataSocket[hubId].emplace_back(sock);
           //CERR << "connected to " << remote.address << ":" << remote.dataPort << ", now have " << m_remoteDataSocket[hubId].size() << " connections" << std::endl;
           lock.unlock();

           startThread();
           startThread();
           msgForward(sock, Local);
       });

       lock.lock();
   }

   while (!m_connectingSockets.empty() && m_remoteDataSocket[hubId].size() < numconn) {
       lock.unlock();
       usleep(10000);
       lock.lock();
   }

   timer.cancel();
   m_connectingSockets.clear();

   if (m_remoteDataSocket[hubId].size() >= numconn) {
       CERR << "connected to hub (data) at " << remote.address << ":" << remote.dataPort << " with " << m_remoteDataSocket[hubId].size() << " parallel connections" << std::endl;
   } else {
       CERR << "WARNING: connected to hub (data) at " << remote.address << ":" << remote.dataPort << " with ONLY " << m_remoteDataSocket[hubId].size() << " parallel connections" << std::endl;
   }

   return !m_remoteDataSocket[hubId].empty();
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getLocalDataSock(const message::Message &msg) {

   lock_guard lock(m_mutex);
   auto it = m_localDataSocket.find(msg.destRank());
   if (it == m_localDataSocket.end()) {
       CERR << "did not find local destination socket for " << msg << std::endl;
       return nullptr;
   }
   return it->second;
}

std::shared_ptr<boost::asio::ip::tcp::socket> DataProxy::getRemoteDataSock(const message::Message &msg) {

    int hubId = idToHub(msg.destId());

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
   if (it == m_remoteDataSocket.end()) {
       CERR << "did not find remote destination socket for " << msg << std::endl;
       return nullptr;
   }
   auto &socks = it->second;
   if (socks.empty()) {
       CERR << "no remote destination socket connected for " << msg << std::endl;
       return nullptr;
   }
   int idx = msg.rank();
   if (idx < 0)
       idx = 0;
   idx %= socks.size();
   return socks[idx].sock;
}

}
