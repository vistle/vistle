#ifndef VISTLE_HUB_H
#define VISTLE_HUB_H

#include <boost/shared_ptr.hpp>
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
   bool sendMessage(boost::shared_ptr<socket> sock, const message::Message &msg);
   unsigned short port() const;
   const std::string &name() const;

   bool handleMessage(const message::Message &msg,
         boost::shared_ptr<boost::asio::ip::tcp::socket> sock = boost::shared_ptr<boost::asio::ip::tcp::socket>());
#if 0
   bool handleLocalData(const message::Message &msg,
         boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   bool handleRemoteData(const message::Message &msg,
         boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
#endif

   bool sendManager(const message::Message &msg, int hub = message::Id::LocalHub);
   bool sendMaster(const message::Message &msg);
   bool sendSlaves(const message::Message &msg, bool returnToSender=false);
   bool sendSlave(const message::Message &msg, int id);
   bool sendHub(const message::Message &msg, int id);
   bool sendUi(const message::Message &msg);
   bool sendRemoteData(const message::Message &msg, int hubId);
   bool sendRemoteData(const char *buf, size_t n, int hubId);
   bool sendLocalData(const message::Message &msg, int rank);
   bool sendLocalData(const char *buf, size_t n, int hubId);

   const StateTracker &stateTracker() const;
   StateTracker &stateTracker();

   int idToHub(int id) const;
   int id() const;

private:
   struct Slave;

   void hubReady();
   bool connectToMaster(const std::string &host, unsigned short port);
#if 0
   bool connectRemoteData(int hubId);
   boost::shared_ptr<boost::asio::ip::tcp::socket> getRemoteDataSock(int hubId);
   void addLocalData(int rank, boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addRemoteData(int hub, boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
#endif
   bool startUi(const std::string &uipath);
   bool startServer();
   bool startAccept();
   void handleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void handleAccept(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void addSocket(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, message::Identify::Identity ident = message::Identify::UNKNOWN);
   bool removeSocket(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addClient(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addSlave(const std::string &name, boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void slaveReady(Slave &slave);
   bool startCleaner();
   bool startManager();

   unsigned short m_port;
   boost::asio::io_service m_ioService;
   boost::shared_ptr<acceptor> m_acceptor;

   std::map<boost::shared_ptr<boost::asio::ip::tcp::socket>, message::Identify::Identity> m_sockets;
   std::set<boost::shared_ptr<boost::asio::ip::tcp::socket>> m_clients;

   boost::shared_ptr<DataProxy> m_dataProxy;
   TunnelManager m_tunnelManager;
   StateTracker m_stateTracker;
   UiManager m_uiManager;

   std::map<process_handle, int> m_processMap;
   bool m_managerConnected;

   std::string m_bindir;
   std::string m_scriptPath;
   bool m_quitting;

   AvailableMap m_availableModules;

   bool m_isMaster;
   boost::shared_ptr<boost::asio::ip::tcp::socket> m_masterSocket;
#if 0
   typedef std::map<int, boost::shared_ptr<boost::asio::ip::tcp::socket>> DataSocketMap;
   DataSocketMap m_localDataSocket; // sockets to local MPI ranks
   DataSocketMap m_remoteDataSocket; // sockets to remote hubs
#endif
   struct Slave {
      boost::shared_ptr<boost::asio::ip::tcp::socket> sock;
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
   bool handlePriv(const message::Barrier &barrier);
   bool handlePriv(const message::BarrierReached &reached);
   bool handlePriv(const message::RequestTunnel &tunnel);
};

}
#endif
