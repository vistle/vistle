#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>

#include <core/assert.h>
#include <util/hostname.h>
#include <util/directory.h>
#include <util/spawnprocess.h>
#include <util/sleep.h>
#include <core/object.h>
#include <core/message.h>
#include <core/tcpmessage.h>
#include <core/messagerouter.h>
#include <core/porttracker.h>
#include <core/statetracker.h>
#include <core/shm.h>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "proxy.h"

//#define DEBUG_DISTRIBUTED

namespace asio = boost::asio;
using std::shared_ptr;
using namespace vistle;
using message::Router;
using message::Id;
namespace dir = vistle::directory;

#define CERR std::cerr << "Proxy " << m_hubId << ": " 

Proxy *hub_instance = nullptr;

Proxy::Proxy()
: m_port(31093)
, m_masterPort(m_port)
, m_masterHost("localhost")
, m_acceptor(new boost::asio::ip::tcp::acceptor(m_ioService))
, m_stateTracker("Proxy state")
, m_managerConnected(false)
, m_quitting(false)
, m_isMaster(true)
, m_slaveCount(0)
, m_hubId(Id::Invalid)
, m_traceMessages(message::INVALID)
{

   vassert(!hub_instance);
   hub_instance = this;

   message::DefaultSender::init(m_hubId, 0);
}

Proxy::~Proxy() {

   m_dataProxy.reset();
   hub_instance = nullptr;
}

Proxy &Proxy::the() {

   return *hub_instance;
}

const StateTracker &Proxy::stateTracker() const {

   return m_stateTracker;
}

StateTracker &Proxy::stateTracker() {

   return m_stateTracker;
}

unsigned short Proxy::port() const {

   return m_port;
}

bool Proxy::sendMessage(shared_ptr<socket> sock, const message::Message &msg) {

   bool result = true;
   try {
      result = message::send(*sock, msg);
   } catch(const boost::system::system_error &err) {
      std::cerr << "exception: err.code()=" << err.code() << std::endl;
      result = false;
      if (err.code() == boost::system::errc::broken_pipe) {
         std::cerr << "broken pipe" << std::endl;
      } else {
         throw(err);
      }
   }
   return result;
}

bool Proxy::startServer() {

   while (!m_acceptor->is_open()) {

      asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), m_port);
      try {
         m_acceptor->open(endpoint.protocol());
      } catch (const boost::system::system_error &err) {
         if (err.code() == boost::system::errc::address_family_not_supported) {
            endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port);
            m_acceptor->open(endpoint.protocol());
         } else {
            throw(err);
         }
      }
      m_acceptor->set_option(acceptor::reuse_address(true));
      try {
         m_acceptor->bind(endpoint);
      } catch(const boost::system::system_error &err) {
         if (err.code() == boost::system::errc::address_in_use) {
            m_acceptor->close();
            ++m_port;
            continue;
         }
         throw(err);
      }
      m_acceptor->listen();
      CERR << "listening for connections on port " << m_port << std::endl;
      startAccept();
   }

   return true;
}

bool Proxy::startAccept() {
   
   shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_ioService));
   addSocket(sock);
   m_acceptor->async_accept(*sock, [this, sock](boost::system::error_code ec) {
         handleAccept(sock, ec);
         });
   return true;
}

void Proxy::handleAccept(shared_ptr<asio::ip::tcp::socket> sock, const boost::system::error_code &error) {

   if (error) {
      removeSocket(sock);
      return;
   }

   addClient(sock);

   message::Identify ident(message::Identify::REQUEST);
   sendMessage(sock, ident);

   startAccept();
}

void Proxy::addSocket(shared_ptr<asio::ip::tcp::socket> sock, message::Identify::Identity ident) {

   bool ok = m_sockets.insert(std::make_pair(sock, ident)).second;
   vassert(ok);
   (void)ok;
}

bool Proxy::removeSocket(shared_ptr<asio::ip::tcp::socket> sock) {

   return m_sockets.erase(sock) > 0;
}

void Proxy::addClient(shared_ptr<asio::ip::tcp::socket> sock) {

   //CERR << "new client" << std::endl;
   m_clients.insert(sock);
}

void Proxy::addSlave(const std::string &name, shared_ptr<asio::ip::tcp::socket> sock) {

   vassert(m_isMaster);
   ++m_slaveCount;
   const int slaveid = Id::MasterHub - m_slaveCount;
   m_slaves[slaveid].sock = sock;
   m_slaves[slaveid].name = name;
   m_slaves[slaveid].ready = false;
   m_slaves[slaveid].id = slaveid;

   message::SetId set(slaveid);
   sendMessage(sock, set);
}

void Proxy::slaveReady(Slave &slave) {

   CERR << "slave hub '" << slave.name << "' ready" << std::endl;

   vassert(m_isMaster);
   vassert(!slave.ready);

   auto state = m_stateTracker.getState();
   for (auto &m: state) {
      sendMessage(slave.sock, m);
   }
   slave.ready = true;
}

bool Proxy::dispatch() {

   m_ioService.poll();

   bool ret = true;
   bool work = false;
   size_t avail = 0;
   do {
      std::shared_ptr<boost::asio::ip::tcp::socket> sock;
      avail = 0;
      for (auto &s: m_clients) {
         boost::asio::socket_base::bytes_readable command(true);
         s->io_control(command);
         if (command.get() > avail) {
            avail = command.get();
            sock = s;
         }
      }
      boost::system::error_code error;
      if (sock) {
          work = true;
          handleWrite(sock, error);
      }
   } while (avail > 0);

   if (m_quitting) {
       ret = false;
   }

   vistle::adaptive_wait(work, this);

   return ret;
}

void Proxy::handleWrite(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error) {

    if (error) {
        CERR << "write error on socket: " << error.message() << std::endl;
        m_quitting = true;
        return;
    }

    message::Identify::Identity senderType = message::Identify::UNKNOWN;
    auto it = m_sockets.find(sock);
    if (it != m_sockets.end())
       senderType = it->second;
    message::Buffer msg;
    message::error_code ec;
    if (message::recv(*sock, msg, ec)) {
       bool ok = true;
       if (senderType == message::Identify::UI) {
          //ok = m_uiManager.handleMessage(msg, sock);
       } else if (senderType == message::Identify::LOCALBULKDATA) {
          //ok = handleLocalData(msg, sock);
          CERR << "invalid identity on socket: " << senderType << std::endl;
          ok = false;
       } else if (senderType == message::Identify::REMOTEBULKDATA) {
          //ok = handleRemoteData(msg, sock);
          CERR << "invalid identity on socket: " << senderType << std::endl;
          ok = false;
       } else {
          ok = handleMessage(msg, sock);
       }
       if (!ok) {
          m_quitting = true;
          //break;
       }
    } else if (ec) {
        CERR << "receiving message failed: " << ec.message();
        m_quitting = true;
    }
}

bool Proxy::sendMaster(const message::Message &msg) {

   vassert(!m_isMaster);
   if (m_isMaster) {
      return false;
   }

   int numSent = 0;
   for (auto &sock: m_sockets) {
      if (sock.second == message::Identify::HUB) {
         ++numSent;
         sendMessage(sock.first, msg);
      }
   }
   vassert(numSent == 1);
   return numSent == 1;
}

bool Proxy::sendManager(const message::Message &msg, int hub) {

   if (hub == Id::LocalHub || hub == m_hubId || (hub == Id::MasterHub && m_isMaster)) {
      vassert(m_managerConnected);
      if (!m_managerConnected) {
         CERR << "sendManager: no connection, cannot send " << msg << std::endl;
         return false;
      }

      int numSent = 0;
      for (auto &sock: m_sockets) {
         if (sock.second == message::Identify::MANAGER) {
            ++numSent;
            sendMessage(sock.first, msg);
         }
      }
      vassert(numSent == 1);
      return numSent == 1;
   } else {
      sendHub(msg, hub);
   }
   return true;
}

bool Proxy::sendSlaves(const message::Message &msg, bool returnToSender) {

   vassert(m_isMaster);
   if (!m_isMaster)
      return false;

   int senderHub = msg.senderId();
   if (senderHub > 0) {
      //std::cerr << "mod id " << senderHub;
      senderHub = m_stateTracker.getHub(senderHub);
      //std::cerr << " -> hub id " << senderHub << std::endl;
   }

   for (auto &slave: m_slaves) {
      if (slave.first != senderHub || returnToSender) {
         //std::cerr << "to slave id: " << sock.first << " (!= " << senderHub << ")" << std::endl;
         if (slave.second.ready)
            sendMessage(slave.second.sock, msg);
      }
   }
   return true;
}

bool Proxy::sendHub(const message::Message &msg, int hub) {

   if (hub == m_hubId)
      return true;

   if (!m_isMaster && hub == Id::MasterHub) {
      sendMaster(msg);
      return true;
   }

   for (auto &slave: m_slaves) {
      if (slave.first == hub) {
         sendMessage(slave.second.sock, msg);
         return true;
      }
   }

   return false;
}

bool Proxy::sendSlave(const message::Message &msg, int dest) {

   vassert(m_isMaster);
   if (!m_isMaster)
      return false;

   auto it = m_slaves.find(dest);
   if (it == m_slaves.end())
      return false;

   return sendMessage(it->second.sock, msg);
}

int Proxy::idToHub(int id) const {

   if (id >= Id::ModuleBase)
      return m_stateTracker.getHub(id);

   if (id == Id::LocalManager || id == Id::LocalHub)
       return m_hubId;

   return m_stateTracker.getHub(id);
}

int Proxy::id() const {

    return m_hubId;
}

void Proxy::hubReady() {
   vassert(m_managerConnected);
   if (m_isMaster) {
   } else {
      message::AddHub hub(m_hubId, m_name);
      hub.setDestId(Id::ForBroadcast);
      hub.setPort(m_port);
      hub.setDataPort(m_dataProxy->port());

      for (auto &sock: m_sockets) {
         if (sock.second == message::Identify::HUB) {
            try {
               auto addr = sock.first->local_endpoint().address();
               if (addr.is_v6()) {
                  hub.setAddress(addr.to_v6());
               } else if (addr.is_v4()) {
                  hub.setAddress(addr.to_v4());
               }
            } catch (std::bad_cast &except) {
               CERR << "AddHub: failed to convert local address to v6" << std::endl;
            }
         }
      }

      sendMaster(hub);
   }
}

bool Proxy::handleMessage(const message::Message &recv, shared_ptr<asio::ip::tcp::socket> sock) {

   using namespace vistle::message;

   message::Buffer buf(recv);
   Message &msg = buf;
   message::Identify::Identity senderType = message::Identify::UNKNOWN;
   auto it = m_sockets.find(sock);
   if (sock) {
      vassert(it != m_sockets.end());
      if (it != m_sockets.end())
         senderType = it->second;
   }

   if (senderType == Identify::UI)
      msg.setSenderId(m_hubId);

   bool masterAdded = false;
   switch (msg.type()) {
      case message::IDENTIFY: {

         auto &id = static_cast<const Identify &>(msg);
         CERR << "ident msg: " << id.identity() << std::endl;
         if (id.identity() != Identify::UNKNOWN && id.identity() != Identify::REQUEST) {
            it->second = id.identity();
         }
         switch(id.identity()) {
            case Identify::REQUEST: {
               if (senderType == Identify::REMOTEBULKDATA) {
                  sendMessage(sock, Identify(Identify::REMOTEBULKDATA, m_hubId));
               } else if (senderType == Identify::LOCALBULKDATA) {
                  sendMessage(sock, Identify(Identify::LOCALBULKDATA, -1));
               } else if (m_isMaster) {
                  sendMessage(sock, Identify(Identify::HUB, m_name));
               } else {
                  sendMessage(sock, Identify(Identify::SLAVEHUB, m_name));
               }
               break;
            }
            case Identify::MANAGER: {
               vassert(!m_managerConnected);
               m_managerConnected = true;

               if (m_hubId != Id::Invalid) {
                  message::SetId set(m_hubId);
                  sendMessage(sock, set);
                  if (m_hubId <= Id::MasterHub) {
                     auto state = m_stateTracker.getState();
                     for (auto &m: state) {
                        sendMessage(sock, m);
                     }
                  }
                  hubReady();
               }
               break;
            }
            case Identify::UI: {
               break;
            }
            case Identify::HUB: {
               vassert(!m_isMaster);
               CERR << "master hub connected" << std::endl;
               break;
            }
            case Identify::SLAVEHUB: {
               vassert(m_isMaster);
               CERR << "slave hub '" << id.name() << "' connected" << std::endl;
               addSlave(id.name(), sock);
               break;
            }
            default: {
               CERR << "invalid identity: " << id.identity() << std::endl;
               break;
            }
         }
         return true;
      }
      case message::ADDHUB: {
         auto &mm = static_cast<const AddHub &>(msg);
         if (m_isMaster) {
            auto it = m_slaves.find(mm.id());
            if (it == m_slaves.end()) {
               break;
            }
            auto &slave = it->second;
            slaveReady(slave);
         } else {
            if (mm.id() == Id::MasterHub) {
               CERR << "received AddHub for master" << std::endl;
               auto m = mm;
               m.setAddress(m_masterSocket->remote_endpoint().address());
               m_stateTracker.handle(m, true);
               masterAdded = true;
            } else {
                m_stateTracker.handle(mm, true);
            }
         }
         if (mm.id() > m_hubId) {
             m_dataProxy->connectRemoteData(mm.id());
         }
         break;
      }
      default:
         break;
   }

   const int dest = idToHub(msg.destId());
   const int sender = idToHub(msg.senderId());

   {
       bool mgr=false, ui=false, master=false, slave=false;

       if (m_hubId == Id::MasterHub) {
           if (msg.destId() == Id::ForBroadcast)
               msg.setDestId(Id::Broadcast);
       }
       bool track = Router::the().toTracker(msg, senderType) && !masterAdded;
       m_stateTracker.handle(msg, track);

       if (Router::the().toManager(msg, senderType, sender)
               || (Id::isModule(msg.destId()) && dest == m_hubId)) {

           sendManager(msg);
           mgr = true;
       }
       if (Id::isModule(msg.destId()) && !Id::isHub(dest)) {
           CERR << "failed to translate module id " << msg.destId() << " to hub" << std::endl;
       } else  if (!Id::isHub(dest)) {
           if (Router::the().toMasterHub(msg, senderType, sender)) {
               sendMaster(msg);
               master = true;
           }
           if (Router::the().toSlaveHub(msg, senderType, sender)) {
               sendSlaves(msg, msg.destId()==Id::Broadcast /* return to sender */ );
               slave = true;
           }
           vassert(!(slave && master));
       } else {
           if (dest != m_hubId) {
               if (m_isMaster) {
                   sendHub(msg, dest);
                   //sendSlaves(msg);
                   slave = true;
               } else if (sender == m_hubId) {
                   sendMaster(msg);
                   master = true;
               }
           }
       }

       if (m_traceMessages == message::ANY || msg.type() == m_traceMessages
#ifdef DEBUG_DISTRIBUTED
               || msg.type() == message::ADDOBJECT
               || msg.type() == message::ADDOBJECTCOMPLETED
#endif
               ) {
           if (track) std::cerr << "t"; else std::cerr << ".";
           if (mgr) std::cerr << "m" ;else std::cerr << ".";
           if (ui) std::cerr << "u"; else std::cerr << ".";
           if (slave) { std::cerr << "s"; } else if (master) { std::cerr << "M"; } else std::cerr << ".";
           std::cerr << " " << msg << std::endl;
       }
   }

   if (Router::the().toHandler(msg, senderType)) {

      switch(msg.type()) {

         case message::SETID: {

            vassert(!m_isMaster);
            auto &set = static_cast<const SetId &>(msg);
            m_hubId = set.getId();
            m_dataProxy->setHubId(m_hubId);
            message::DefaultSender::init(m_hubId, 0);
            Router::init(message::Identify::SLAVEHUB, m_hubId);
            CERR << "got hub id " << m_hubId << std::endl;
            if (m_managerConnected) {
               sendManager(set);
            }
            if (m_managerConnected) {
               auto state = m_stateTracker.getState();
               for (auto &m: state) {
                  sendMessage(sock, m);
               }
               hubReady();
            }
            break;
         }

         case message::QUIT: {

            std::cerr << "hub: got quit: " << msg << std::endl;
            auto &quit = static_cast<const Quit &>(msg);
            if (senderType == message::Identify::MANAGER) {
               if (m_isMaster)
                  sendSlaves(quit);
            } else if (senderType == message::Identify::HUB) {
               sendManager(quit);
            } else if (senderType == message::Identify::UI) {
               if (m_isMaster)
                  sendSlaves(quit);
               else
                  sendMaster(quit);
               sendManager(quit);
            } else {
               sendSlaves(quit);
               sendManager(quit);
            }
            m_quitting = true;
            break;
         }

         case message::REQUESTTUNNEL: {
            auto &tunnel = static_cast<const RequestTunnel &>(msg);
            handlePriv(tunnel);
            break;
         }

         default: {
            break;
         }
      }
   }

   return true;
}

bool Proxy::init(int argc, char *argv[]) {

   m_name = hostname();

   namespace po = boost::program_options;
   po::options_description desc("usage");
   desc.add_options()
      ("help,h", "show this message")
      ("hub,c", po::value<std::string>(), "hub to connect to")
      ;
   po::variables_map vm;
   try {
      po::positional_options_description popt;
      po::store(po::command_line_parser(argc, argv).options(desc).positional(popt).run(), vm);
      po::notify(vm);
   } catch (std::exception &e) {
      CERR << e.what() << std::endl;
      CERR << desc << std::endl;
      return false;
   }

   if (vm.count("help")) {
      CERR << desc << std::endl;
      return false;
   }

   if (vm.count("") > 0) {
      CERR << desc << std::endl;
      return false;
   }

   startServer();
   m_dataProxy.reset(new DataProxy(m_stateTracker, m_port+1));

   std::string uiCmd = "vistle_gui";

   if (vm.count("hub") > 0) {
      m_isMaster = false;
      m_masterHost = vm["hub"].as<std::string>();
      auto colon = m_masterHost.find(':');
      if (colon != std::string::npos) {
         m_masterPort = boost::lexical_cast<unsigned short>(m_masterHost.substr(colon+1));
         m_masterHost = m_masterHost.substr(0, colon);
      }

      if (!connectToMaster(m_masterHost, m_masterPort)) {
         CERR << "failed to connect to master at " << m_masterHost << ":" << m_masterPort << std::endl;
         return false;
      }
   } else {
      // this is the master hub
      m_hubId = Id::MasterHub;
      m_dataProxy->setHubId(m_hubId);
      message::DefaultSender::init(m_hubId, 0);
      Router::init(message::Identify::HUB, m_hubId);

      message::AddHub master(m_hubId, m_name);
      master.setPort(m_port);
      master.setDataPort(m_dataProxy->port());
      m_stateTracker.handle(master);
      m_masterPort = m_port;
   }

   std::string port = boost::lexical_cast<std::string>(this->port());
   std::string dataPort = boost::lexical_cast<std::string>(m_dataProxy->port());

   return true;
}

bool Proxy::connectToMaster(const std::string &host, unsigned short port) {

   vassert(!m_isMaster);

   asio::ip::tcp::resolver resolver(m_ioService);
   asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
   asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
   boost::system::error_code ec;
   m_masterSocket.reset(new boost::asio::ip::tcp::socket(m_ioService));
   asio::connect(*m_masterSocket, endpoint_iterator, ec);
   if (ec) {
      CERR << "could not establish connection to " << host << ":" << port << std::endl;
      return false;
   }

#if 0
   message::Identify ident(message::Identify::SLAVEHUB, m_name);
   sendMessage(m_masterSocket, ident);
#endif
   addSocket(m_masterSocket, message::Identify::HUB);
   addClient(m_masterSocket);

   CERR << "connected to master at " << host << ":" << port << std::endl;
   return true;
}

bool Proxy::handlePriv(const message::RequestTunnel &tunnel) {

    return m_tunnelManager.processRequest(tunnel);
}

int main(int argc, char *argv[]) {

   try {
      Proxy proxy;
      if (!proxy.init(argc, argv)) {
         return 1;
      }

      while(proxy.dispatch())
         ;
   } catch (vistle::exception &e) {
      std::cerr << "Proxy: fatal exception: " << e.what() << std::endl << e.where() << std::endl;
      return 1;
   } catch (std::exception &e) {
      std::cerr << "Proxy: fatal exception: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}
