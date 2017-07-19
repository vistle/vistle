#ifndef VISTLE_HUB_H
#define VISTLE_HUB_H

#include <memory>
#include <boost/asio.hpp>
#include <core/statetracker.h>
#include <net/tunnel.h>
#include <net/dataproxy.h>

namespace vistle {

class Proxy {

 public:
   typedef boost::asio::ip::tcp::socket socket;
   typedef boost::asio::ip::tcp::acceptor acceptor;
   
   static Proxy &the();

   Proxy();
   ~Proxy();

   bool init(int argc, char *argv[]);
   bool dispatch();
   bool sendMessage(std::shared_ptr<socket> sock, const message::Message &msg);
   unsigned short port() const;
   const std::string &name() const;

   bool handleMessage(const message::Message &msg,
         std::shared_ptr<boost::asio::ip::tcp::socket> sock = std::shared_ptr<boost::asio::ip::tcp::socket>());
   bool sendManager(const message::Message &msg, int hub = message::Id::LocalHub);
   bool sendMaster(const message::Message &msg);
   bool sendSlaves(const message::Message &msg, bool returnToSender=false);
   bool sendSlave(const message::Message &msg, int id);
   bool sendHub(const message::Message &msg, int id);

   const StateTracker &stateTracker() const;
   StateTracker &stateTracker();

   int idToHub(int id) const;
   int id() const;

private:
   struct Slave;

   void hubReady();
   bool connectToMaster(const std::string &host, unsigned short port);
   bool startServer();
   bool startAccept();
   void handleWrite(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void handleAccept(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);
   void addSocket(std::shared_ptr<boost::asio::ip::tcp::socket> sock, message::Identify::Identity ident = message::Identify::UNKNOWN);
   bool removeSocket(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addClient(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void addSlave(const std::string &name, std::shared_ptr<boost::asio::ip::tcp::socket> sock);
   void slaveReady(Slave &slave);

   unsigned short m_port, m_masterPort;
   std::string m_masterHost;
   boost::asio::io_service m_ioService;
   std::shared_ptr<acceptor> m_acceptor;

   std::map<std::shared_ptr<boost::asio::ip::tcp::socket>, message::Identify::Identity> m_sockets;
   std::set<std::shared_ptr<boost::asio::ip::tcp::socket>> m_clients;

   std::shared_ptr<DataProxy> m_dataProxy;
   TunnelManager m_tunnelManager;
   StateTracker m_stateTracker;

   bool m_managerConnected;

   std::string m_prefix;
   bool m_quitting;

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

   int m_traceMessages;

   bool handlePriv(const message::RequestTunnel &tunnel);
};

}
#endif
