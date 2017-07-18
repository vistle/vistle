#ifndef VISTLE_HUB_H
#define VISTLE_HUB_H

#include <memory>
#include <boost/asio.hpp>
#include <core/porttracker.h>
#include <core/statetracker.h>
#include <util/spawnprocess.h>
#include <util/directory.h>
#include "uimanager.h"
#include "tunnel.h"
#include "dataproxy.h"

namespace vistle {

class Hub {

 public:
   typedef boost::asio::ip::tcp::socket socket;
   typedef boost::asio::ip::tcp::acceptor acceptor;
   
   static Hub &the();

   Hub();
   ~Hub();

   bool init(int argc, char *argv[]);
   bool processScript();
   bool dispatch();
   bool sendMessage(std::shared_ptr<socket> sock, const message::Message &msg);
   unsigned short port() const;
   vistle::process_handle launchProcess(const std::vector<std::string>& argv);
   const std::string &name() const;

   bool handleMessage(const message::Message &msg,
         std::shared_ptr<boost::asio::ip::tcp::socket> sock = std::shared_ptr<boost::asio::ip::tcp::socket>());
   bool sendManager(const message::Message &msg, int hub = message::Id::LocalHub);
   bool sendMaster(const message::Message &msg);
   bool sendSlaves(const message::Message &msg, bool returnToSender=false);
   bool sendSlave(const message::Message &msg, int id);
   bool sendHub(const message::Message &msg, int id);
   bool sendUi(const message::Message &msg);

   const StateTracker &stateTracker() const;
   StateTracker &stateTracker();

   int idToHub(int id) const;
   int id() const;

private:
   struct Slave;

   void hubReady();
   bool connectToMaster(const std::string &host, unsigned short port);
   bool startUi(const std::string &uipath);
   bool startServer();
   bool startAccept();
   void handleWrite(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void handleAccept(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void addSocket(std::shared_ptr<boost::asio::ip::tcp::socket> sock, message::Identify::Identity ident = message::Identify::UNKNOWN);
   bool removeSocket(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addClient(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addSlave(const std::string &name, std::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void slaveReady(Slave &slave);
   bool startCleaner();
   bool startManager();

   unsigned short m_port, m_masterPort;
   std::string m_masterHost;
   boost::asio::io_service m_ioService;
   std::shared_ptr<acceptor> m_acceptor;

   std::map<std::shared_ptr<boost::asio::ip::tcp::socket>, message::Identify::Identity> m_sockets;
   std::set<std::shared_ptr<boost::asio::ip::tcp::socket>> m_clients;

   std::shared_ptr<DataProxy> m_dataProxy;
   TunnelManager m_tunnelManager;
   StateTracker m_stateTracker;
   UiManager m_uiManager;

   std::map<process_handle, int> m_processMap;
   bool m_managerConnected;

   std::string m_prefix;
   std::string m_scriptPath;
   bool m_quitting;

   AvailableMap m_availableModules;
   std::vector<AvailableModule> m_localModules;

   bool m_isMaster;
   std::shared_ptr<boost::asio::ip::tcp::socket> m_masterSocket;
   struct Slave {
      std::shared_ptr<boost::asio::ip::tcp::socket> sock;
      std::string name;
      bool ready = false;
      int id = 0;
   };
   std::map<int, Slave> m_slaves;
   int m_slaveCount;
   int m_hubId;
   std::string m_name;

   int m_moduleCount;
   int m_traceMessages;

   int m_execCount;

   bool m_barrierActive;
   unsigned m_barrierReached;
   message::uuid_t m_barrierUuid;

   bool handlePriv(const message::Execute &exec);
   bool handlePriv(const message::CancelExecute &cancel);
   bool handlePriv(const message::Barrier &barrier);
   bool handlePriv(const message::BarrierReached &reached);
   bool handlePriv(const message::RequestTunnel &tunnel);
   bool handlePriv(const message::Connect &conn);
   bool handlePriv(const message::Disconnect &disc);

   std::vector<message::Buffer> m_queue;
   bool handleQueue();
};

}
#endif
