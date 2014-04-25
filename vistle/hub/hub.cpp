/*
 * Visualization Testing Laboratory for Exascale Computing (VISTLE)
 */

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sstream>

#include <util/findself.h>
#include <util/spawnprocess.h>
#include <core/object.h>
#include <control/executor.h>
#include <core/message.h>
#include <core/tcpmessage.h>
#include <core/porttracker.h>
#include <core/statetracker.h>
#include <core/shm.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "uimanager.h"
#include "uiclient.h"
#include "hub.h"

#ifdef HAVE_PYTHON
#include "pythoninterpreter.h"
#endif

namespace asio = boost::asio;
using boost::shared_ptr;
using namespace vistle;

#define CERR std::cerr << "Hub: "

Hub *hub_instance = nullptr;

Hub::Hub()
: m_port(31093)
, m_acceptor(m_ioService)
, m_stateTracker(&m_portTracker)
, m_uiManager(*this, m_stateTracker)
, m_idCount(0)
, m_managerConnected(false)
, m_quitting(false)
{

   assert(!hub_instance);
   hub_instance = this;

   message::DefaultSender::init(0, 0);

   startServer();
}

Hub::~Hub() {

   hub_instance = nullptr;
}

Hub &Hub::the() {

   return *hub_instance;
}

const StateTracker &Hub::stateTracker() const {

   return m_stateTracker;
}

StateTracker &Hub::stateTracker() {

   return m_stateTracker;
}

unsigned short Hub::port() const {

   return m_port;
}

bool Hub::sendMessage(shared_ptr<socket> sock, const message::Message &msg) {

   return message::send(*sock, msg);
}

bool Hub::startServer() {

   while (!m_acceptor.is_open()) {

      asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), m_port);
      m_acceptor.open(endpoint.protocol());
      m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
      try {
         m_acceptor.bind(endpoint);
      } catch(const boost::system::system_error &err) {
         if (err.code() == boost::system::errc::address_in_use) {
            m_acceptor.close();
            ++m_port;
            continue;
         }
         throw(err);
      }
      m_acceptor.listen();
      CERR << "listening for connections on port " << m_port << std::endl;
      startAccept();
   }

   return true;
}

bool Hub::startAccept() {
   
   shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_ioService));
   addSocket(sock);
   m_acceptor.async_accept(*sock, boost::bind<void>(&Hub::handleAccept, this, sock, asio::placeholders::error));
   return true;
}

void Hub::handleAccept(shared_ptr<asio::ip::tcp::socket> sock, const boost::system::error_code &error) {

   if (error) {
      removeSocket(sock);
      return;
   }

   addClient(sock);

   message::Identify ident;
   sendMessage(sock, ident);

   startAccept();
}

void Hub::addSocket(shared_ptr<asio::ip::tcp::socket> sock) {

   m_sockets.insert(std::make_pair(sock, message::Identify::UNKNOWN));
}

bool Hub::removeSocket(shared_ptr<asio::ip::tcp::socket> sock) {

   return m_sockets.erase(sock) > 0;
}

void Hub::addClient(shared_ptr<asio::ip::tcp::socket> sock) {

   //CERR << "new client" << std::endl;
   m_clients.insert(sock);
}

bool Hub::dispatch() {

   m_ioService.poll();

   bool ret = true;
   bool wait = true;
   for (auto &sock: m_clients) {

      message::Identify::Identity senderType = message::Identify::UNKNOWN;
      auto it = m_sockets.find(sock);
      if (it != m_sockets.end())
         senderType = it->second;
      if (senderType != message::Identify::UI) {
         boost::asio::socket_base::bytes_readable command(true);
         sock->io_control(command);
         if (command.get() > 0) {
            char buf[message::Message::MESSAGE_SIZE];
            message::Message &msg = *reinterpret_cast<message::Message *>(buf);
            bool received = false;
            if (message::recv(*sock, msg, received) && received) {
               if (received)
                  wait = false;
               if (!handleMessage(sock, msg)) {
                  ret = false;
                  break;
               }
            }
         }
      }
   }

   if (auto pid = vistle::try_wait()) {
      wait = false;
      auto it = m_processMap.find(pid);
      if (it == m_processMap.end()) {
         CERR << "unknown process with PID " << pid << " exited" << std::endl;
      } else {
         CERR << "process with id " << it->second << " (PID " << pid << ") exited" << std::endl;
         m_processMap.erase(it);
      }
   }

   if (m_quitting) {
      if (m_processMap.empty()) {
         ret = false;
      } else {
         CERR << "still " << m_processMap.size() << " processes running" << std::endl;
         for (const auto &proc: m_processMap) {
            std::cerr << "   id: " << proc.second << ", pid: " << proc.first << std::endl;
         }
      }
   }

   if (wait)
      usleep(10000);

   return ret;
}

bool Hub::sendMaster(const message::Message &msg) {

   if (!m_managerConnected)
      return false;

   int numSent = 0;
   for (auto &sock: m_sockets) {
      if (sock.second == message::Identify::MANAGER) {
         ++numSent;
         sendMessage(sock.first, msg);
      }
   }
   assert(numSent == 1);
   return numSent == 1;
}

bool Hub::sendSlaves(const message::Message &msg) {

   for (auto &sock: m_sockets) {
      if (sock.second == message::Identify::HUB)
         sendMessage(sock.first, msg);
   }
   return true;
}

bool Hub::sendUi(const message::Message &msg) {

#if 0
   for (auto &sock: m_sockets) {
      if (sock.second == message::Identify::UI)
         sendMessage(sock.first, msg);
   }
#else
   m_uiManager.sendMessage(msg);
#endif
   return true;
}

bool Hub::handleUiMessage(const message::Message &msg) {

   return sendMaster(msg);
}

bool Hub::handleMessage(shared_ptr<asio::ip::tcp::socket> sock, const message::Message &msg) {

   using namespace vistle::message;

   message::Identify::Identity senderType = message::Identify::UNKNOWN;
   auto it = m_sockets.find(sock);
   if (sock) {
      assert(it != m_sockets.end());
      senderType = it->second;
   }

   if (senderType == message::Identify::MANAGER)
      m_stateTracker.handle(msg);

   switch(msg.type()) {

      case Message::IDENTIFY: {

         auto &id = static_cast<const Identify &>(msg);
         it->second = id.identity();
         switch(id.identity()) {
            case Identify::MANAGER: {
               assert(!m_managerConnected);
               m_managerConnected = true;
               processScript();
               break;
            }
            case Identify::UI: {
               ++m_idCount;
               boost::shared_ptr<UiClient> c(new UiClient(m_uiManager, m_idCount, sock));
               m_uiManager.addClient(c);
               break;
            }
         }
         break;
      }

      case Message::EXEC: {

         auto &exec = static_cast<const Exec &>(msg);
         if (senderType == message::Identify::MANAGER) {
            std::string executable = exec.pathname();
            auto args = exec.args();
            std::vector<std::string> argv;
            argv.push_back("spawn_vistle.sh");
            argv.push_back(executable);
            for (auto &a: args) {
               argv.push_back(a);
            }
            auto pid = spawn_process("spawn_vistle.sh", argv);
            if (pid) {
               //std::cerr << "started " << executable << " with PID " << pid << std::endl;
               m_processMap[pid] = exec.moduleId();
            } else {
               std::cerr << "program " << executable << " failed to start" << std::endl;
            }
         }
         break;
      }

      case Message::QUIT: {

         //std::cerr << "hub: broadcasting message: " << msg << std::endl;
         auto &quit = static_cast<const Quit &>(msg);
         if (senderType == message::Identify::MANAGER) {
            m_uiManager.requestQuit();
            sendSlaves(quit);
            m_quitting = true;
            return true;
         } else {
            sendMaster(quit);
         }
         break;
      }

      default: {
         //CERR << "msg: " << msg << std::endl;
         if (senderType == message::Identify::MANAGER) {
            sendUi(msg);
            sendSlaves(msg);
         } else if (senderType == message::Identify::HUB) {
            sendMaster(msg);
         } else {
            CERR << "message from unknow sender: " << msg << std::endl;
         }
         break;
      }
      
   }

   return true;
}

std::string hostname() {

   // process with the smallest rank on each host allocates shm
   const size_t HOSTNAMESIZE = 256;

   char hostname[HOSTNAMESIZE];
   gethostname(hostname, HOSTNAMESIZE-1);
   hostname[HOSTNAMESIZE-1] = '\0';
   return hostname;
}

bool Hub::init(int argc, char *argv[]) {

   std::string bindir = getbindir(argc, argv);
   {
      // start UI
      bool start_gui = true;
      bool start_tui = false;
      if (argc > 1) {
         bool haveUiArg = true;
         std::string arg = argv[1];
         if (arg == "-no-ui" || arg == "-batch") {
            start_gui = false;
            start_tui = false;
         } else if (arg == "-gui") {
            start_gui = true;
            start_tui = false;
         } else if (arg == "-tui") {
            start_gui = false;
            start_tui = true;
         } else {
            haveUiArg = false;
         }

         if (haveUiArg) {
            --argc;
            ++argv;
         }
      }

      if (start_gui || start_tui) {
         std::string cmd = "vistle_gui";
         if (start_tui)
            cmd = "blower";
         m_uiPath = bindir + "/" + cmd;
      }
   }

   startUi();

   if (argc > 1)
      m_scriptPath = argv[1];

   std::stringstream s;
   s << this->port();
   std::string port = s.str();

   // start manager on cluster
   std::string cmd = bindir + "/vistle_manager";
   std::vector<std::string> args;
   args.push_back("spawn_vistle.sh");
   args.push_back(cmd);
   args.push_back(hostname());
   args.push_back(port);
   auto pid = vistle::spawn_process("spawn_vistle.sh", args);
   if (!pid) {
      CERR << "failed to spawn Vistle manager " << std::endl;
      exit(1);
   }
   m_processMap[pid] = 0;

   return true;
}

bool Hub::startUi() {

   std::stringstream s;
   s << this->port();
   std::string port = s.str();

   std::vector<std::string> args;
   args.push_back(m_uiPath);
   args.push_back("-from-vistle");
   args.push_back("localhost");
   args.push_back(port);
   auto pid = vistle::spawn_process(m_uiPath, args);
   if (!pid) {
      CERR << "failed to spawn UI " << m_uiPath << std::endl;
      return false;
   }

   m_processMap[pid] = -1; // will be id of first UI

   return true;
}

bool Hub::processScript() {

#ifdef HAVE_PYTHON
   m_uiManager.lockUi(true);
   if (!m_scriptPath.empty()) {
      PythonInterpreter inter(m_scriptPath);
      while(inter.check()) {
         dispatch();
      }
   }
   m_uiManager.lockUi(false);
   return true;
#else
   return false;
#endif
}

int main(int argc, char *argv[]) {

   Hub hub;
   hub.init(argc, argv);

   while(hub.dispatch())
      ;

   return 0;
}
